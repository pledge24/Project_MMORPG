@echo off
setlocal
pushd %~dp0
protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto

PacketHandlerGenerator.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=S_ --send=C_
PacketHandlerGenerator.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=C_ --send=S_

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enum.pb.h					"../../../GameServer/Protocol"
XCOPY /Y Enum.pb.cc					"../../../GameServer/Protocol"
XCOPY /Y Struct.pb.h				"../../../GameServer/Protocol"
XCOPY /Y Struct.pb.cc				"../../../GameServer/Protocol"
XCOPY /Y Protocol.pb.h				"../../../GameServer/Protocol"
XCOPY /Y Protocol.pb.cc				"../../../GameServer/Protocol"
XCOPY /Y ServerPacketHandler.h		"../../../GameServer/Main"

XCOPY /Y Enum.pb.h					"../../../DummyClient/Protocol"
XCOPY /Y Enum.pb.cc					"../../../DummyClient/Protocol"
XCOPY /Y Struct.pb.h				"../../../DummyClient/Protocol"
XCOPY /Y Struct.pb.cc				"../../../DummyClient/Protocol"
XCOPY /Y Protocol.pb.h				"../../../DummyClient/Protocol"
XCOPY /Y Protocol.pb.cc				"../../../DummyClient/Protocol"
XCOPY /Y ClientPacketHandler.h		"../../../DummyClient/Main"

XCOPY /Y Enum.pb.h					"../../../../P1/Source/P1/Network"
XCOPY /Y Enum.pb.cc					"../../../../P1/Source/P1/Network"
XCOPY /Y Struct.pb.h				"../../../../P1/Source/P1/Network"
XCOPY /Y Struct.pb.cc				"../../../../P1/Source/P1/Network"
XCOPY /Y Protocol.pb.h				"../../../../P1/Source/P1/Network"
XCOPY /Y Protocol.pb.cc				"../../../../P1/Source/P1/Network"
XCOPY /Y ClientPacketHandler.h		"../../../../P1/Source/P1"

XCOPY /Y Protocol.proto				"../../../../P1/Source/P1/Network"
XCOPY /Y Struct.proto				"../../../../P1/Source/P1/Network"
XCOPY /Y Enum.proto					"../../../../P1/Source/P1/Network"

DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

echo [INFO] 완료되었습니다.
PAUSE
popd
endlocal