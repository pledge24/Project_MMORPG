import argparse
import openpyxl
import CellParser
import json
from pathlib import Path

def snake_to_pascal(snake_str):
    return ''.join(word.capitalize() for word in snake_str.split('_'))

def snake_to_camel(snake_str: str) -> str:
    parts = snake_str.split('_')
    return parts[0].lower() + ''.join(word.capitalize() for word in parts[1:])

def main():

    arg_parser = argparse.ArgumentParser(description = 'ExcelToJsonConverter')

    # 같은 디렉토리에 있는 엑셀 파일 경로를 얻는다.
    current_file = Path(__file__)
    base_dir = current_file.parents[0]
    target_file = base_dir/'test1.xlsx' # 엑셀 파일 경로

    # argparse 객체에 CLI 명령줄에서 사용가능한 인자 종류를 추가한다.
    arg_parser.add_argument('--path', type=str, default=target_file.resolve(), help='excel file path')
    arg_parser.add_argument('--c_output', type=str, default='C_TestJsonfile.json', help='json file path')
    arg_parser.add_argument('--s_output', type=str, default='S_TestJsonfile.json', help='json file path')

    # 명령줄 파싱
    args = arg_parser.parse_args()

    # 엑셀 파일을 연다.
    workbook = openpyxl.load_workbook(args.path, data_only=True)

    # Init
    parser = CellParser.CellParser()
    clientData = []
    serverData = []

    # 각 데이터시트를 json으로 변환(첫 번째 행을 헤더로 사용)
    for sheet in workbook.worksheets:

        # 헤더 정보 추출
        headers =[cell.value for cell in sheet[1]]
        authorizations = [cell.value for cell in sheet[2]]

        for row in sheet.iter_rows(min_row=3, values_only=True):
            row_client_data = {}
            row_server_data = {}

            for i, cell_value in enumerate(row):
                parsed_value = parser.parse_cell(cell_value)

                # 헤더 이름 읽어오기
                headerName = headers[i]

                # "TRUE", "FALSE"를 True, False로 변환
                if(parsed_value == "TRUE"):
                    parsed_value = True

                if(parsed_value == "FALSE"):
                    parsed_value = False

                # Server, Client가 각각 저장해야할 부분만 저장.
                if(authorizations[i] == "BOTH" or authorizations[i] == "CLIENT"):
                    # 언리얼을 "Name"이라는 필드가 반드시 있어야해서 이리함.
                    if(headerName == "template_id"):
                        row_client_data["Name"] = parsed_value
                    
                    row_client_data[snake_to_pascal(headerName)] = parser.convert_keys(parsed_value, mode="pascal")

                if(authorizations[i] == "BOTH" or authorizations[i] == "SERVER"):
                    row_server_data[snake_to_camel(headerName)] = parser.convert_keys(parsed_value, mode="camel")

            clientData.append(row_client_data)
            serverData.append(row_server_data)

    # 저장
    with open(args.c_output, 'w+', encoding='UTF8') as f:
        json.dump(clientData, f, indent=4, ensure_ascii=False)

    print("/******************************************")
    print("| Complete to generate ",args.c_output,"     |")
    print("*******************************************/")

    with open(args.s_output, 'w+', encoding='UTF8') as f:
        json.dump(serverData, f, indent=4, ensure_ascii=False)

    print("/******************************************")
    print("| Complete to generate ",args.s_output,"     |")
    print("*******************************************/")

    return

if __name__ == '__main__':
	try:
		main()
	except Exception as e:
		print(f"오류가 발생했습니다: {e}")