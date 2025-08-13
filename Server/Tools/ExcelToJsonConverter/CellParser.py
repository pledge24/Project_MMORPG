import json

class CellParser:
    """
    엑셀 데이터를 JSON으로 변환하는 클래스
    JavaScript 객체 리터럴 형식의 셀 데이터를 처리
    """
    def __init__(self):
        pass
    
    def clean_excel_string(self, text):
        """
        엑셀에서 복사한 문자열의 불필요한 따옴표와 이스케이프 문자 제거
        """
        if not isinstance(text, str):
            return text
        
        text = text.strip()
        
        # 양쪽 끝의 따옴표 제거
        if text.startswith('"') and text.endswith('"'):
            text = text[1:-1]
        
        # 이스케이프된 따옴표 복원
        text = text.replace('\\"', '"')
        
        return text
    
    def parse_object_literal(self, text):
        """
        JavaScript 객체 리터럴 스타일의 문자열을 파이썬 딕셔너리로 파싱
        예: 'Type:"Kill", Object:"Wolf", Count:5' -> {"Type": "Kill", "Object": "Wolf", "Count": 5}
        """
        result = {}
        i = 0
        
        while i < len(text):
            # 공백 건너뛰기
            while i < len(text) and text[i].isspace():
                i += 1
            if i >= len(text):
                break
                
            # 키 읽기
            key_start = i
            while i < len(text) and text[i] != ':':
                i += 1
            if i >= len(text):
                break
                
            key = text[key_start:i].strip()
            i += 1  # ':' 건너뛰기
            
            # 공백 건너뛰기
            while i < len(text) and text[i].isspace():
                i += 1
            if i >= len(text):
                break
                
            # 값 읽기
            if text[i] == '"':
                # 따옴표로 둘러싸인 문자열
                i += 1  # 시작 따옴표 건너뛰기
                value_start = i
                while i < len(text) and text[i] != '"':
                    i += 1
                value = text[value_start:i]
                if i < len(text):
                    i += 1  # 끝 따옴표 건너뛰기
                result[key] = value
            elif text[i] == '{':
                # 중괄호로 시작하는 중첩 객체
                brace_count = 1
                i += 1
                value_start = i
                while i < len(text) and brace_count > 0:
                    if text[i] == '{':
                        brace_count += 1
                    elif text[i] == '}':
                        brace_count -= 1
                    i += 1
                value_text = text[value_start:i-1]  # 중괄호 제외
                result[key] = self.parse_object_literal(value_text)
            else:
                # 일반 값 (쉼표까지)
                value_start = i
                while i < len(text) and text[i] != ',':
                    i += 1
                value_text = text[value_start:i].strip()
                
                # 값 타입 판단
                if value_text.isdigit() or (value_text.startswith('-') and value_text[1:].isdigit()):
                    result[key] = int(value_text)
                elif '.' in value_text and value_text.replace('.', '').replace('-', '').isdigit():
                    result[key] = float(value_text)
                elif value_text.lower() == 'true':
                    result[key] = True
                elif value_text.lower() == 'false':
                    result[key] = False
                elif value_text.lower() == 'null':
                    result[key] = None
                else:
                    result[key] = value_text
            
            # 쉼표 건너뛰기
            while i < len(text) and text[i] in ', ':
                i += 1
        
        return result
    
    def parse_cell(self, cell_value):
        """
        셀 값을 JSON 객체로 변환하는 함수
        JavaScript 객체 리터럴 형식을 JSON으로 변환
        """
        if cell_value is None or not isinstance(cell_value, str):
            return cell_value
        
        # 엑셀 문자열 정리
        cleaned_value = self.clean_excel_string(cell_value)
        
        # 콜론이 없으면 일반 문자열로 처리
        if ':' not in cleaned_value:
            return cleaned_value
        
        try:
            # 1. 이미 유효한 JSON인지 확인
            return json.loads(cleaned_value)
        except (json.JSONDecodeError, ValueError):
            pass
        
        try:
            # 2. 중괄호 추가 후 JSON 파싱 시도
            return json.loads("{" + cleaned_value + "}")
        except (json.JSONDecodeError, ValueError):
            pass
        
        try:
            # 3. 커스텀 파서로 JavaScript 객체 리터럴 파싱
            return self.parse_object_literal(cleaned_value)
        except Exception:
            return cleaned_value
