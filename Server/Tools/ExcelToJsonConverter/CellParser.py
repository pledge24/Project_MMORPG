import json

class CellParser:
    def __init__(self):
        pass
      
    def parse_cell(self, value):
        """
        셀 값이 JSON 포맷인지 여부를 검사 후 변환.
        JSON이 아니면 문자열/숫자 그대로 반환.
        """
        if value is None:
            return None
    
        if isinstance(value, (int, float)):  # 숫자 그대로 반환
            return value
    
        if isinstance(value, str):
            try:
                # JSON 형식이면 변환
                return json.loads(value)
            except json.JSONDecodeError:
                # JSON 형식이 아니면 문자열 그대로
                return value
    
        return value