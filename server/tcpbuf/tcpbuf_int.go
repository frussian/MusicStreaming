package tcpbuf

import (
	"fmt"
	"io"
	"net"
)

type streamData struct {
	stream *Stream
	data []byte
}

func connectToServer(stream *Stream) bool {
	return false
}

func listenRoutine(server *ServerListener) {
	for {
		c, err := server.listener.AcceptTCP()
		if err != nil {
			server.errChan <- err
			continue
		}
		server.newConns <- c
	}
}

func (instance *TCPBuf) loopServer(server *ServerListener) {
	stop := false
	for {
		select {
		case err := <-server.errChan:
			evt := instance.newEvent(nil, ERROR_LISTEN, server.addr, err)
			res := server.cb(server.userCtx, evt)
			if res != OK {
				return
			}
		case conn := <- server.newConns:
			addr, _ := net.ResolveTCPAddr(conn.RemoteAddr().Network(),
				conn.RemoteAddr().String())
			evt := instance.newEvent(nil, NEW_CONN, addr, nil)
			instance.currentListener = server
			instance.currentAddr = addr
			instance.currentConn = conn
			instance.accepted = false

			server.cb(server.userCtx, evt)

			if !instance.accepted {
				conn.Close()
			}
			instance.currentListener = nil
			instance.currentAddr = nil
			instance.currentConn = nil
			instance.accepted = false
		default:
			stop = true
		}

		if stop {
			break
		}
	}
}

func readRoutine(stream *Stream) {
	stream.instance.logger.Info("started read routine")
	for {
		tmpBuf := make([]byte, 256)
		n, err := stream.conn.Read(tmpBuf)
		//stream.instance.logger.Info(fmt.Sprintf("read %d", n))
		if err != nil {
		 	if err != io.EOF {
				stream.instance.logger.Error(err.Error())
			} else {
				stream.instance.logger.Info(fmt.Sprintf("closing %s stream", stream.addr.String()))
			}
			stream.readChan <- streamData{stream, nil}
			break
		}
		stream.readChan <- streamData{stream, tmpBuf[:n]}
	}
	stream.instance.logger.Info("closing read routine")
}

func (stream *Stream) tryRead() {
	if stream.inTail == 0 {
		return
	}
	evt := stream.instance.newEvent(stream.inBuf[stream.inHead:stream.inTail],
		READ, stream.addr, nil)
	res := stream.cb(stream.userCtx, evt)
	if res >= 0 {
		read := int(res)
		if stream.inHead + read > stream.inTail {
			oldRead := read
			read = stream.inTail - stream.inHead
			stream.instance.logger.Warn("too much read",
				"stream", stream, "read", oldRead,
				"read avail", read)
		}
		stream.inHead += read
		if stream.inHead == stream.inTail {
			stream.instance.logger.Debug("clear in buf",
				"stream", stream,
				"head", stream.inHead, "tail", stream.inTail)
			stream.inHead = 0
			stream.inTail = 0
		}
		if stream.inHead > 0 {
			stream.instance.logger.Debug("moving input buffer",
				"stream", stream, "offset", stream.inHead)
			copy(stream.inBuf, stream.inBuf[stream.inHead:])
			stream.inTail -= stream.inHead
			stream.inHead = 0
		}
	} else if res == CLOSE {
		stream.handleCloseConn()
		return
	}
}

func (stream *Stream) tryWrite() {
	evt := stream.instance.newEvent(stream.outBuf[stream.outTail:], WRITE,
		stream.addr, nil)
	res := stream.cb(stream.userCtx, evt)
	if res > 0 {
		toWrite := int(res)
		if stream.outTail + toWrite >= len(stream.outBuf) {
			toWrite = len(stream.outBuf) - stream.outTail
			//TODO: log warning
		}
		stream.outTail += toWrite
		if stream.outTail == len(stream.outBuf) {
			stream.instance.logger.Debug("moving output buffer",
				"stream", stream, "offset", stream.outHead)
			copy(stream.outBuf, stream.outBuf[stream.outHead:])
			stream.outTail -= stream.outHead
			stream.outHead = 0
		}

	} else if res == CLOSE {
		stream.handleCloseConn()
		return
	}

	//TODO: maybe move this to goroutine
	if stream.outHead == stream.outTail {
		return
	}
	stream.instance.logger.Debug("start writing")
	n, _ := stream.conn.Write(stream.outBuf[stream.outHead:stream.outTail])
	stream.instance.logger.Debug("finish writing")
	stream.outHead += n
	if stream.outHead >= stream.outTail {
		stream.outHead = 0
		stream.outTail = 0
	} else {
		n := stream.outHead
		stream.instance.logger.Debug("moving output buffer",
			"stream", stream, "offset", stream.outHead)
		copy(stream.outBuf, stream.outBuf[stream.outHead:stream.outTail])
		stream.outHead = 0
		stream.outTail -= n
	}
	//
}

func (stream *Stream) handleCloseConn() {
	i := stream.instance.findStream(stream)
	if i == -1 {
		return
	}
	evt := stream.instance.newEvent(nil, CLOSING, stream.addr, nil)
	stream.cb(stream.userCtx, evt)
	stream.conn.Close()

	stream.instance.logger.Info(fmt.Sprintf("%v", stream.instance.streams))
	stream.instance.logger.Info(fmt.Sprintf("%p", stream))
	stream.instance.removeStream(i)
}

func (instance *TCPBuf) removeStream(i int) {
	//res := make([]*Stream, 0, len(instance.streams)-1)
	instance.streams = append(instance.streams[:i], instance.streams[i+1:]...)
}

func (instance *TCPBuf) findStream(stream *Stream) int {
	for i, s := range instance.streams {
		if s == stream {
			return i
		}
	}
	return -1
}

func (instance *TCPBuf) processStreamRead(data streamData) {
	stream := data.stream
	tmpBuf := data.data
	if tmpBuf == nil {
		stream.handleCloseConn()
		return
	}
	n := len(tmpBuf)
	if stream.inTail+n >= len(stream.inBuf) {
		instance.logger.Warn("more in data than available",
			"stream", stream, "offset", stream.inHead)
		if stream.inHead > 0 {
			//trying to move to free up space
			instance.logger.Debug("moving input buffer",
				"stream", stream, "offset", stream.inHead)
			copy(stream.inBuf, stream.inBuf[stream.inHead:])
		}
		delta := stream.inHead
		stream.inTail -= delta
		stream.inHead = 0
		if n > delta {
			//TODO: still overflow
			n = delta
		}
	}

	copy(stream.inBuf[stream.inTail:], tmpBuf)
	stream.inTail += n
	stream.tryRead()
}
//
//func (instance *TCPBuf) loopStream(stream *Stream) {
//	stop := false
//	for {
//		select {
//		case tmpBuf, ok := <-stream.readChan:
//			if !ok {
//				stream.handleCloseConn()
//				return
//			}
//			n := len(tmpBuf)
//			if stream.inTail + n >= len(stream.inBuf) {
//				if stream.inHead > 0 {
//					//trying to move to free up space
//					copy(stream.inBuf, stream.inBuf[stream.inHead:])
//				}
//				delta := stream.inHead
//				stream.inTail -= delta
//				stream.inHead = 0
//				if n > delta {
//					//TODO: still overflow
//					n = delta
//				}
//			}
//			copy(stream.inBuf[stream.inTail:], tmpBuf)
//			stream.inTail += n
//			stream.tryRead()
//		default:
//			stop = true
//		}
//		if stop {
//			break
//		}
//	}
//
//	stream.tryRead()
//	stream.tryWrite()
//}

func (instance *TCPBuf) newEvent(data []byte, kind EventType, addr *net.TCPAddr, err error) Event {
	return Event{
		Data: data,
		Kind: kind,
		Addr: addr,
		Err:  err,
		Instance: instance,
	}
}