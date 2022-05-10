package tcpbuf

import (
	"fmt"
	log "gopkg.in/inconshreveable/log15.v2"
	"net"
	"time"
)

type EventType int
const (
	CONNECTED EventType = iota  //for successful client connect
	LISTENING  //server starts listening (in ServerListener callback)
	READ
	WRITE
	CLOSING
	ERROR_CONN  //
	ERROR_LISTEN  //error listen (in ServerListener callback)
	ERROR_READ
	ERROR_WRITE
	NEW_CONN  //new connection (in ServerListener callback)
)

type RetType int
const (
	OK RetType = -iota
	RECONNECT
	ERROR
	CLOSE
)

type Event struct {
	Data []byte
	Kind EventType
	Addr *net.TCPAddr
	Err error
	Instance *TCPBuf
}

type Callback func(interface{}, Event) RetType
type ServerListener struct {
	host string
	port int
	cb Callback
	inSize, outSize int
	userCtx interface{}

	active bool
	listener *net.TCPListener
	addr *net.TCPAddr
	newConns chan *net.TCPConn
	errChan chan error

	instance *TCPBuf
}

func (instance *TCPBuf) NewServerListener(host string, port int, cb Callback,
						inSize, outSize int, userCtx interface{}) *ServerListener {
	serv := &ServerListener{
		host: host,
		port: port,
		cb:   cb,
		inSize: inSize,
		outSize: outSize,
		userCtx: userCtx,
		active: false,
		newConns: make(chan *net.TCPConn),
		errChan: make(chan error),
		instance: instance,
	}
	instance.listeners = append(instance.listeners, serv)
	return serv
}

func (server *ServerListener) StartServer() bool {
	ips, err := net.LookupIP(server.host)
	if err != nil {
		evt := server.instance.newEvent(nil, ERROR_LISTEN, nil, err)
		server.cb(server.userCtx, evt)
		return false
	}
	addr := &net.TCPAddr{
		IP:   ips[0],
		Port: server.port,
		Zone: "",
	}
	server.addr = addr

	listener, err := net.ListenTCP("tcp", addr)
	//operr, ok := err.(net.OpError)
	if err != nil {
		evt := server.instance.newEvent(nil, ERROR_LISTEN, addr, err)
		server.cb(server.userCtx, evt)
		return false
	}

	server.listener = listener
	server.active = true
	go listenRoutine(server)
	evt := server.instance.newEvent(nil, LISTENING, addr, nil)
	server.cb(server.userCtx, evt)
	return true
}

type Stream struct {
	userCtx interface{}
	cb Callback
	isServer bool

	inBuf, outBuf []byte
	inHead, inTail int
	outHead, outTail int

	addr *net.TCPAddr
	conn *net.TCPConn

	readChan chan streamData

	instance *TCPBuf
}

func (s *Stream) String() string {
	return fmt.Sprintf("%p", s)
}

type TCPBuf struct {
	currentListener *ServerListener
	currentConn *net.TCPConn
	currentAddr *net.TCPAddr
	accepted bool

	listeners []*ServerListener
	streams []*Stream
	readChan chan streamData
	logger log.Logger
}

func NewTCPBuf(logger log.Logger, readChanSize int) *TCPBuf {
	return &TCPBuf{listeners: nil,
		streams: nil,
		logger: logger,
		readChan: make(chan streamData, readChanSize)}
}

//only for clients
func (instance *TCPBuf) NewTCPStream(userCtx interface{}, host string, port, inSize, outSize int, cb Callback) *Stream {
	ips, _ := net.LookupIP(host)
	addr := &net.TCPAddr{
		IP:   ips[0],
		Port: port,
		Zone: "",
	}
	stream := &Stream{
		userCtx: userCtx,
		cb:      cb,
		inBuf:    make([]byte, inSize),
		outBuf:   make([]byte, outSize),
		addr: addr,
		isServer: false,
		readChan: instance.readChan,
		instance: instance,
	}
	instance.streams = append(instance.streams, stream)
	go readRoutine(stream)
	return stream
}

//connects to server
func ConnectTCPStream(stream *Stream) bool {
	return false
}

func (instance *TCPBuf) Loop(ms time.Duration) {
	timeout := time.After(ms)
	for _, stream := range instance.streams {
		stream.tryWrite()
		stream.tryRead()
	}
	for _, listener := range instance.listeners {
		if listener.active {
			instance.loopServer(listener)
		}
	}
	stop := false
	for {
		select {
		case <-timeout:
			stop = true
		case data := <-instance.readChan:
			instance.processStreamRead(data)
		}
		if stop {
			break
		}
	}
}

func (instance *TCPBuf) AcceptStream(userCtx interface{},
									cb Callback) *Stream {
	if instance.currentListener == nil ||
		instance.currentConn == nil ||
		instance.currentAddr == nil {
		return nil
	}
	stream := &Stream{
		userCtx:  userCtx,
		cb:       cb,
		isServer: true,
		inBuf:    make([]byte, instance.currentListener.inSize),
		outBuf:   make([]byte, instance.currentListener.outSize),
		addr:     instance.currentAddr,
		conn:     instance.currentConn,
		readChan: instance.readChan,
		instance: instance,
	}
	instance.streams = append(instance.streams, stream)
	instance.accepted = true
	go readRoutine(stream)
	return stream
}
