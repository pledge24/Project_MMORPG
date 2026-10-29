import re
from pathlib import Path
from typing import NamedTuple

# 스키마 파일의 패킷 번호 범위 선언. 예: `// @packet_id_range 200 299`
RANGE_PATTERN = re.compile(r'^//\s*@packet_id_range\s+(\d+)\s+(\d+)\s*$')

# 최상위 메시지 선언. 중첩 메시지는 들여 쓰므로 걸리지 않는다
MESSAGE_PATTERN = re.compile(r'^message\s+([A-Za-z_][A-Za-z0-9_]*)')

# 핸들러 표(GPacketHandler)의 칸 수가 UINT16_MAX라 마지막 번호 65535는 쓸 수 없다
MAX_PACKET_ID = 65534

# 스키마 파일은 BOM 없는 UTF-8이다
SCHEMA_ENCODING = 'utf-8'


class ProtoParseError(Exception):
	pass


class PacketIdRange(NamedTuple):
	"""스키마 파일 하나의 패킷 번호 범위. 양 끝을 포함한다."""
	first: int
	last: int

	def capacity(self):
		return self.last - self.first + 1

	def overlaps(self, other):
		return self.first <= other.last and other.first <= self.last


class SchemaFile(NamedTuple):
	"""읽은 스키마 파일 하나. 패킷이 없는 파일은 범위가 없을 수 있다."""
	file_name: str		# 예: Lobby.proto
	id_range: PacketIdRange | None

	@property
	def stem(self):
		return Path(self.file_name).stem


class ProtoParser():
	"""
	스키마 파일 여럿에서 패킷을 모으고 번호를 매긴다.

	패킷은 이름이 수신 또는 송신 접두사로 시작하는 최상위 메시지다. 번호는 파일마다 선언한 범위의 첫 번호부터
	그 파일 안의 선언 순서로 매기므로, 한 파일에 패킷을 끼워 넣어도 다른 파일의 번호는 밀리지 않는다.
	"""
	def __init__(self, recv_prefix, send_prefix):
		self.recv_pkt = []	# 수신 패킷 목록
		self.send_pkt = [] # 송신 패킷 목록
		self.total_pkt = [] # 모든 패킷 목록
		self.schemas = [] # 읽은 스키마 파일. 읽은 순서
		self.recv_prefix = recv_prefix
		self.send_prefix = send_prefix

	def parse_proto(self, path):
		"""스키마 파일 하나를 읽어 패킷을 더한다. 범위가 틀리거나 모자라면 ProtoParseError."""
		path = Path(path)
		with open(path, 'r', encoding=SCHEMA_ENCODING) as f:
			lines = f.readlines()

		id_range = None
		range_line_no = None
		packets = [] # (이름, 수신 패킷인지)
		for line_no, line in enumerate(lines, start=1):
			line = line.rstrip('\r\n')

			range_match = RANGE_PATTERN.match(line)
			if range_match:
				if id_range is not None:
					raise ProtoParseError(f'{path.name}:{line_no}: 패킷 번호 범위를 두 번 선언했다')
				id_range = PacketIdRange(int(range_match.group(1)), int(range_match.group(2)))
				range_line_no = line_no
				continue

			message_match = MESSAGE_PATTERN.match(line)
			if message_match is None:
				continue

			pkt_name = message_match.group(1)
			if pkt_name.startswith(self.recv_prefix):
				packets.append((pkt_name, True))
			elif pkt_name.startswith(self.send_prefix):
				packets.append((pkt_name, False))

		if id_range is None:
			if packets:
				raise ProtoParseError(f'{path.name}: 패킷이 있는데 패킷 번호 범위(// @packet_id_range <첫 번호> <끝 번호>)가 없다')
			self.schemas.append(SchemaFile(path.name, None))
			return

		self._check_range(path.name, id_range, range_line_no)
		self.schemas.append(SchemaFile(path.name, id_range))

		if len(packets) > id_range.capacity():
			raise ProtoParseError(f'{path.name}: 패킷이 {len(packets)}개라 범위 {id_range.first}~{id_range.last}({id_range.capacity()}개)에 들어가지 않는다')

		for offset, (pkt_name, is_recv) in enumerate(packets):
			packet = Packet(pkt_name, id_range.first + offset)
			(self.recv_pkt if is_recv else self.send_pkt).append(packet)
			self.total_pkt.append(packet)

	def _check_range(self, file_name, id_range, line_no):
		if id_range.first < 1 or id_range.last > MAX_PACKET_ID or id_range.first > id_range.last:
			raise ProtoParseError(f'{file_name}:{line_no}: 패킷 번호 범위 {id_range.first}~{id_range.last}가 1~{MAX_PACKET_ID} 안의 오름차순 범위가 아니다')

		for other in self.schemas:
			if other.id_range is not None and id_range.overlaps(other.id_range):
				raise ProtoParseError(f'{file_name}:{line_no}: 패킷 번호 범위 {id_range.first}~{id_range.last}가 {other.file_name}의 범위 {other.id_range.first}~{other.id_range.last}와 겹친다')


class Packet:
	def __init__(self, name, id):
		self.name = name
		self.id = id
