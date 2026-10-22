import argparse
import sys
import jinja2
import ProtoParser
from pathlib import Path

def main():

	# CLI 인자를 담당하는 argparse 객체를 하나 만든다.
	arg_parser = argparse.ArgumentParser(description = 'PacketHandlerGenerator')

	# argparse 객체에 CLI 명령줄에서 사용가능한 인자 종류를 추가한다.
	arg_parser.add_argument('--path', type=str, action='append', required=True, help='스키마 파일 경로. 파일마다 한 번씩 준다')
	arg_parser.add_argument('--output', type=str, default='TestPacketHandler', help='출력 파일 이름이자 핸들러 클래스 이름')
	arg_parser.add_argument('--recv', type=str, default='C_', help='recv convention')
	arg_parser.add_argument('--send', type=str, default='S_', help='send convention')
	arg_parser.add_argument('--templates', type=str, required=True, help='템플릿 폴더')
	arg_parser.add_argument('--out-dir', type=str, required=True, help='출력 폴더')

	# 명령줄(ex. PacketHandlerGenerator.exe --path=Lobby.proto --path=Item.proto)에서 제공된 인자를 파싱
	args = arg_parser.parse_args()

	# 스키마 파일마다 패킷 이름을 분류하고 파일의 범위 안에서 번호를 매긴다.
	parser = ProtoParser.ProtoParser(args.recv, args.send)
	for path in args.path:
		parser.parse_proto(path)

	# 템플릿 파일이 들어있는 디렉토리 위치 지정.
	file_loader = jinja2.FileSystemLoader(args.templates)

	# jinja2 환경 객체를 생성. 템플릿에 없는 변수를 쓰면 빈 문자열 대신 실패한다.
	env = jinja2.Environment(loader=file_loader, undefined=jinja2.StrictUndefined)

	# 템플릿 폴더에 들어있는 템플릿 코드 'PacketHandler.h'를 가져온다.
	template = env.get_template('PacketHandler.h')

	# 가져온 템플릿에 parse와 output을 넘겨 렌더링하고 최종 출력물을 얻는다.
	output = template.render(parser=parser, output=args.output)

	# 얻어낸 최종 출력물을 원하는 코드에 붙여넣는다(없다면 생성한다.)
	output_path = Path(args.out_dir) / (args.output + '.h')
	with open(output_path, 'w+', encoding='UTF8') as f:
		f.write(output)

	print(f"[생성] {output_path.name}: 패킷 {len(parser.total_pkt)}개")

	return

if __name__ == '__main__':
	# 출력은 언제나 UTF-8로 쓴다. 파이프로 가면 시스템 코드 페이지로 써서, 부르는 스크립트가 한국어 오류 문구를 읽지 못한다.
	# PyInstaller exe는 PYTHONIOENCODING을 무시하므로 여기서 정한다.
	sys.stdout.reconfigure(encoding='utf-8')
	sys.stderr.reconfigure(encoding='utf-8')
	try:
		main()
	except Exception as e:
		# 실패를 출력만 하고 0으로 끝나면 부르는 스크립트가 옛 헤더를 그대로 배포한다.
		print(f"[오류] {e}", file=sys.stderr)
		sys.exit(1)
