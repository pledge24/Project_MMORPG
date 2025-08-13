import argparse
import openpyxl
import CellParser
import json
from pathlib import Path # pathlib 사용 (추천: Python 3.4+)

def main():

    arg_parser = argparse.ArgumentParser(description = 'ExcelToJsonConverter')

    # 같은 디렉토리에 있는 엑셀 파일 경로를 얻는다.
    current_file = Path(__file__)
    base_dir = current_file.parents[0]
    target_file = base_dir/'test.xlsx' # 엑셀 파일 경로

    # argparse 객체에 CLI 명령줄에서 사용가능한 인자 종류를 추가한다.
    arg_parser.add_argument('--path', type=str, default=target_file.resolve(), help='excel file path')
    arg_parser.add_argument('--output', type=str, default='TestJsonfile.json', help='json file path')

    # 명령줄 파싱
    args = arg_parser.parse_args()

    # 엑셀 파일을 연다.
    workbook = openpyxl.load_workbook(args.path)
    sheet = workbook.active 

    # 첫번째 데이터시트를 json으로 변환(첫 번째 행을 헤더로 사용)
    parser = CellParser.CellParser()
    data = []
    headers = [cell.value for cell in sheet[1]]
    for row in sheet.iter_rows(min_row=2, values_only=True):
        row_data = {}
        for i, cell_value in enumerate(row):
            row_data[headers[i]] = parser.parse_cell(cell_value)
        data.append(row_data)

    with open(args.output, 'w+', encoding='UTF8') as f:
        json.dump(data, f, indent=4, ensure_ascii=False)

    print("/******************************************")
    print("| Complete to generate ",args.output,"     |")
    print("*******************************************/")

    return

if __name__ == '__main__':
	try:
		main()
	except Exception as e:
		print(f"오류가 발생했습니다: {e}")