package main

import (
	"encoding/binary"
	"fmt"
	"github.com/gammazero/deque"
	"musicplatform/tcpbuf"
	"musicplatform/proto"
	protobuf "google.golang.org/protobuf/proto"
	"time"
)

type Stream struct {
	reqId uint64
	//bytes [][]byte
	offset int
	size int
	objId int
}

type Conn struct {
	state int
	lenBytes uint32
	messages *deque.Deque
	stateSrv *ServerState
	streams []*Stream
}

func findStream(conn *Conn, reqId uint64) *Stream {
	for _, stream := range conn.streams {
		if stream.reqId == reqId {
			return stream
		}
	}
	return nil
}

func removeStream(conn *Conn, stream *Stream) {
	for i, s := range conn.streams {
		if s.reqId == stream.reqId {
			conn.streams = append(conn.streams[:i], conn.streams[i+1:]...)
			break
		}
	}
}

//func handleState(conn *Conn, data []byte) tcpbuf.RetType {
//	swi
//}

func handleSimpleReq(conn *Conn, req *proto.SimpleReq) *proto.SimpleAns {
	ans := &proto.SimpleAns{}
	switch req.Type {
	case proto.EntityType_BAND:
		ans.Msg = dbSimpleBandReq(conn, req.ReqString)
	}

	return ans
}

func handleTableReq(conn *Conn, req *proto.TableReq) *proto.TableAns {
	var ans *proto.TableAns
	switch req.Type {
	case proto.EntityType_BAND:
		ans = dbTableBandReq(conn.stateSrv, req.First, req.Last, req.Filter)
	case proto.EntityType_ALBUM:
		ans = dbTableAlbumReq(conn.stateSrv, req.First, req.Last, req.Filter)
	case proto.EntityType_SONG:
		ans = dbTableSongReq(conn.stateSrv, req.First, req.Last, req.Filter)
	case proto.EntityType_CONCERT:
		ans = dbTableConcertReq(conn.stateSrv, req.First, req.Last, req.Filter)
	}

	return ans
}

func handleLenState(conn *Conn, data []byte) tcpbuf.RetType {
	if len(data) < 4 {
		return tcpbuf.OK
	}
	logger := conn.stateSrv.logger
	logger.Info("", "got", len(data))
	lenBytes := binary.LittleEndian.Uint32(data[:4])
	conn.lenBytes = lenBytes
	conn.state = 1
	logger.Info(fmt.Sprintf("len %d", lenBytes))
	return 4
}

func handleMsg(conn *Conn, reqWrap *proto.Request) {
	logger := conn.stateSrv.logger
	msg := &proto.Answer{}
	msg.ReqId = reqWrap.ReqId

	switch reqType := reqWrap.Msg.(type) {
	case *proto.Request_SimpleReq: {
		logger.Info("simple req msg")
		//msg.Cancel = true
		req := reqType.SimpleReq
		msg.Msg = &proto.Answer_SimpleAns{SimpleAns: handleSimpleReq(conn, req)}
	}
	case *proto.Request_StreamReq: {
		logger.Info("stream req msg")
		//msg.Cancel = true
		stream := findStream(conn, reqWrap.ReqId)
		if stream != nil {
			if reqWrap.Cancel {
				logger.Info("cancelling request", "reqId", stream.reqId)
				removeStream(conn, stream)
			} else {
				logger.Warn("unexpected stream without cancel", "reqId", stream.reqId)
				msg.Msg = &proto.Answer_StreamAns{StreamAns: &proto.StreamAns{IsFinal: true}}
				msg.Cancel = true
			}
		} else {
			req := reqType.StreamReq
			objId := dbGetObjId(conn, req)
			if objId < 0 {
				logger.Warn("cannot locate objId", "reqId", reqWrap.ReqId, "entity type", req.Type)
				msg.Msg = &proto.Answer_StreamAns{StreamAns: &proto.StreamAns{IsFinal: true}}
				msg.Cancel = true
				break
			}
			stream = &Stream{
				reqId:  reqWrap.ReqId,
				offset: 0,
				size: int(req.SuggestedSize),
				objId:  objId,
			}
			conn.streams = append(conn.streams, stream)
		}
	}
	case *proto.Request_TableReq: {
		req := reqType.TableReq
		logger.Info("table req")
		logger.Info(fmt.Sprintf("%d %d %d %s", req.GetType(), req.GetFirst(),
			req.GetLast(), req.GetFilter()))
		msg.Msg = &proto.Answer_TableAns{TableAns: handleTableReq(conn, req)}
	}
	}

	if msg.Msg == nil {
		return
	}

	bytes, err := protobuf.Marshal(msg)
	if err != nil {
		logger.Error("cannot marshal msg", "reqId", reqWrap.ReqId)
		return
	}
	conn.messages.PushBack(bytes)
	conn.stateSrv.logger.Info(fmt.Sprintf("%v", msg))
}

func handleReqState(conn *Conn, data []byte) tcpbuf.RetType {
	//logger.Info("", "got", len(evt.Data))
	if uint32(len(data)) < conn.lenBytes {
		return tcpbuf.OK
	}
	logger := conn.stateSrv.logger
	reqWrap := &proto.Request{}
	err := protobuf.Unmarshal(data[:conn.lenBytes], reqWrap)
	if err != nil {
		logger.Error(err.Error())
		conn.state = 0
		return tcpbuf.CLOSE
	}

	handleMsg(conn, reqWrap)

	conn.state = 0
	lenBytes := conn.lenBytes
	conn.lenBytes = 0
	return tcpbuf.RetType(lenBytes)
}

func writeReadyPkts(conn *Conn, data []byte) (int, []byte) {
	total := 0
	logger := conn.stateSrv.logger

	for conn.messages.Len() != 0 {
		curr := uint32(4)
		msg, ok := conn.messages.Front().([]byte)
		if !ok {
			logger.Error("not bytes in queue")
			conn.messages.PopFront()
			continue
		}
		curr += uint32(len(msg))
		logger.Info("", "msglen", curr)
		if total + int(curr) > len(data) {
			return total, data
		}
		binary.LittleEndian.PutUint32(data[:4], curr-4)
		copy(data[4:], msg)
		data = data[curr:]
		total += int(curr)
		conn.messages.PopFront()
	}

	return total, data
}

func processStream(conn *Conn, stream *Stream) {

}

func processStreams(conn *Conn) int {
	for _, stream := range conn.streams {
		chunk := dbGetChunk(conn, stream)

		stream.offset += len(chunk)

		isFinal := len(chunk) < stream.size
		msg := &proto.Answer{}
		msg.ReqId = stream.reqId
		msg.Msg = &proto.Answer_StreamAns{
			StreamAns: &proto.StreamAns{IsFinal: isFinal, Data: chunk},
		}

		bytes, err := protobuf.Marshal(msg)
		if err != nil {
			conn.stateSrv.logger.Error("cannot marshal msg", "reqId", stream.reqId)
			return -1
		}
		if isFinal {
			removeStream(conn, stream)
		}
		conn.messages.PushBack(bytes)  //TODO: push front
	}

	return 0
}

func handleWrite(conn *Conn, data []byte) tcpbuf.RetType {
	total := 0
	processStreams(conn)
	wr, data := writeReadyPkts(conn, data)
	total += wr
	return tcpbuf.RetType(total)
}

func streamCB(userCtx interface{}, evt tcpbuf.Event) tcpbuf.RetType {
	conn := userCtx.(*Conn)
	state := conn.stateSrv
	logger := state.logger
	switch evt.Kind {
	case tcpbuf.READ: {
		switch conn.state {
		case 0: return handleLenState(conn, evt.Data)
		case 1: return handleReqState(conn, evt.Data)
		}
		logger.Info(fmt.Sprintf("recv: %s", string(evt.Data)))
		logger.Info(fmt.Sprintf("total: %d", state.read))
		return tcpbuf.RetType(len(evt.Data))
	}
	case tcpbuf.WRITE: {
		return handleWrite(conn, evt.Data)
	}

	case tcpbuf.CLOSING:
		logger.Info(fmt.Sprintf("closing %s", evt.Addr.String()))
	}
	return tcpbuf.OK
}

func servCB(userCtx interface{}, evt tcpbuf.Event) tcpbuf.RetType {
	state := userCtx.(*ServerState)
	logger := state.logger
	switch evt.Kind {
	case tcpbuf.ERROR_LISTEN:
		if evt.Addr == nil {
			logger.Error("invalid address")
		} else {
			logger.Error(evt.Err.Error(), "host", evt.Addr.String())
		}
	case tcpbuf.LISTENING:
		logger.Info("listening on", "host", evt.Addr.String())
	case tcpbuf.NEW_CONN:
		logger.Info("new conn", "remote", evt.Addr.String())
		evt.Instance.AcceptStream(&Conn{0, 0, deque.New(10) , state, nil}, streamCB)
	}
	return tcpbuf.OK
}

func loop(state *ServerState, msLoop time.Duration) {
	state.lib.Loop(msLoop)

}