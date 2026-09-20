@echo off
setlocal
pushd %~dp0
..\Tools\protoc.exe -I=./Schema --cpp_out=./ ./Schema/Enum.proto
..\Tools\protoc.exe -I=./Schema --cpp_out=./ ./Schema/Struct.proto
..\Tools\protoc.exe -I=./Schema --cpp_out=./ ./Schema/Protocol.proto

..\Tools\PacketHandlerGenerator\PacketHandlerGenerator.exe --path=./Schema/Protocol.proto --output=ClientPacketHandler --recv=S_ --send=C_
..\Tools\PacketHandlerGenerator\PacketHandlerGenerator.exe --path=./Schema/Protocol.proto --output=ServerPacketHandler --recv=C_ --send=S_

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enum.pb.h					"../Server/GameServer/Protocol"
XCOPY /Y Enum.pb.cc					"../Server/GameServer/Protocol"
XCOPY /Y Struct.pb.h				"../Server/GameServer/Protocol"
XCOPY /Y Struct.pb.cc				"../Server/GameServer/Protocol"
XCOPY /Y Protocol.pb.h				"../Server/GameServer/Protocol"
XCOPY /Y Protocol.pb.cc				"../Server/GameServer/Protocol"
XCOPY /Y ServerPacketHandler.h		"../Server/GameServer/Main"

XCOPY /Y Enum.pb.h					"../Server/DummyClient/Protocol"
XCOPY /Y Enum.pb.cc					"../Server/DummyClient/Protocol"
XCOPY /Y Struct.pb.h				"../Server/DummyClient/Protocol"
XCOPY /Y Struct.pb.cc				"../Server/DummyClient/Protocol"
XCOPY /Y Protocol.pb.h				"../Server/DummyClient/Protocol"
XCOPY /Y Protocol.pb.cc				"../Server/DummyClient/Protocol"
XCOPY /Y ClientPacketHandler.h		"../Server/DummyClient/Main"

XCOPY /Y Enum.pb.h					"../P1/Source/P1/Network"
XCOPY /Y Enum.pb.cc					"../P1/Source/P1/Network"
XCOPY /Y Struct.pb.h				"../P1/Source/P1/Network"
XCOPY /Y Struct.pb.cc				"../P1/Source/P1/Network"
XCOPY /Y Protocol.pb.h				"../P1/Source/P1/Network"
XCOPY /Y Protocol.pb.cc				"../P1/Source/P1/Network"
XCOPY /Y ClientPacketHandler.h		"../P1/Source/P1/Network"

XCOPY /Y Schema\Protocol.proto				"../P1/Source/P1/Network"
XCOPY /Y Schema\Struct.proto				"../P1/Source/P1/Network"
XCOPY /Y Schema\Enum.proto					"../P1/Source/P1/Network"

DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

echo [INFO] 완료되었습니다.
PAUSE
popd
endlocal