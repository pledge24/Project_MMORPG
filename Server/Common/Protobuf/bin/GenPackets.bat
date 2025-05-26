@echo off
setlocal
pushd %~dp0
protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto

PacketHandlerGenerator.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=S_ --send=C_
PacketHandlerGenerator.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=C_ --send=S_

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enum.pb.h					"../../../GameServer"
XCOPY /Y Enum.pb.cc					"../../../GameServer"
XCOPY /Y Struct.pb.h				"../../../GameServer"
XCOPY /Y Struct.pb.cc				"../../../GameServer"
XCOPY /Y Protocol.pb.h				"../../../GameServer"
XCOPY /Y Protocol.pb.cc				"../../../GameServer"
XCOPY /Y ServerPacketHandler.h		"../../../GameServer"

XCOPY /Y Enum.pb.h					"../../../DummyClient"
XCOPY /Y Enum.pb.cc					"../../../DummyClient"
XCOPY /Y Struct.pb.h				"../../../DummyClient"
XCOPY /Y Struct.pb.cc				"../../../DummyClient"
XCOPY /Y Protocol.pb.h				"../../../DummyClient"
XCOPY /Y Protocol.pb.cc				"../../../DummyClient"
XCOPY /Y ClientPacketHandler.h		"../../../DummyClient"

XCOPY /Y Enum.pb.h					"../../../../P1/Source/P1/Network"
XCOPY /Y Enum.pb.cc					"../../../../P1/Source/P1/Network"
XCOPY /Y Struct.pb.h				"../../../../P1/Source/P1/Network"
XCOPY /Y Struct.pb.cc				"../../../../P1/Source/P1/Network"
XCOPY /Y Protocol.pb.h				"../../../../P1/Source/P1/Network"
XCOPY /Y Protocol.pb.cc				"../../../../P1/Source/P1/Network"
XCOPY /Y ClientPacketHandler.h		"../../../../P1/Source/P1"

DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

echo [INFO] 완료되었습니다.
PAUSE
popd
endlocal