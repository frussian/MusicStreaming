package main

import (
	"encoding/binary"
	"fmt"
	"musicplatform/tcpbuf"
	"musicplatform/proto"
	protobuf "google.golang.org/protobuf/proto"
	"time"
)

type Conn struct {
	state int
	lenBytes uint32
	stateSrv *ServerState
}

//func handleState(conn *Conn, data []byte) tcpbuf.RetType {
//	swi
//}

func handleTableReq(conn *Conn, req *proto.TableReq) {
	ans := dbTableBandReq(conn.stateSrv, req.First, req.Last, req.Filter)
	msg, err := protobuf.Marshal(ans)
	if err != nil {
		conn.stateSrv.logger.Error(err.Error())
		return
	}
	conn.stateSrv.messages.PushBack(msg)
	conn.stateSrv.logger.Info(fmt.Sprintf("%v", msg))
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
	switch reqType := reqWrap.Msg.(type) {
	case *proto.Request_SimpleReq: {
		logger.Info("simple req msg")
	}
	case *proto.Request_StreamReq: {
		logger.Info("stream req msg")
	}
	case *proto.Request_TableReq: {
		req := reqType.TableReq
		logger.Info("table req")
		logger.Info(fmt.Sprintf("%d %d %d %s", req.GetType(), req.GetFirst(),
			req.GetLast(), req.GetFilter()))
		handleTableReq(conn, req)
	}
	}
	conn.state = 0
	lenBytes := conn.lenBytes
	conn.lenBytes = 0
	return tcpbuf.RetType(lenBytes)
}

func streamCB(userCtx interface{}, evt tcpbuf.Event) tcpbuf.RetType {
	conn := userCtx.(*Conn)
	state := conn.stateSrv
	logger := state.logger
	switch evt.Kind {
	case tcpbuf.READ:
		{
			switch conn.state {
			case 0: return handleLenState(conn, evt.Data)
			case 1: return handleReqState(conn, evt.Data)
			}
			logger.Info(fmt.Sprintf("recv: %s", string(evt.Data)))
			logger.Info(fmt.Sprintf("total: %d", state.read))
			return tcpbuf.RetType(len(evt.Data))
		}
	case tcpbuf.WRITE:
		if state.read > 0 {
			//state.read = 0
			//msg := "received"
			//copy(evt.Data, msg)
			//return tcpbuf.RetType(len(msg))
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
			logger.Error("error listening", "host", evt.Addr.String())
		}
	case tcpbuf.LISTENING:
		logger.Info("listening on", "host", evt.Addr.String())
	case tcpbuf.NEW_CONN:
		logger.Info("new conn", "remote", evt.Addr.String())
		evt.Instance.AcceptStream(&Conn{0, 0,  state}, streamCB)
	}
	return tcpbuf.OK
}

func loop(state *ServerState, msLoop time.Duration) {
	state.lib.Loop(msLoop)

}