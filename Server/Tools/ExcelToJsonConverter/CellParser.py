import json
import re

class CellParser:
    def __init__(self):
        pass
      
    def to_pascal_case(self, s: str) -> str:
        parts = re.split(r'[_\-]', s)
        return ''.join(word.capitalize() for word in parts)

    def to_camel_case(self, s: str) -> str:
        parts = re.split(r'[_\-]', s)
        pascal = ''.join(word.capitalize() for word in parts)
        return pascal[0].lower() + pascal[1:] if pascal else pascal

    def convert_keys(self, obj, mode="pascal"):
        if mode == "pascal":
            convert_func = self.to_pascal_case
        elif mode == "camel":
            convert_func = self.to_camel_case
        else:
            raise ValueError("mode must be 'pascal' or 'camel'")

        if isinstance(obj, dict):
            return {convert_func(k): self.convert_keys(v, mode) for k, v in obj.items()}
        elif isinstance(obj, list):
            return [self.convert_keys(v, mode) for v in obj]
        else:
            return obj

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