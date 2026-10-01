---
paths:
  - "P1/Source/**"
---

# UE 클라이언트 코드

- 호출자를 검색할 때는 `_Implementation` 접미사를 붙인 이름과 뗀 이름을 둘 다 검색한다. UE RPC와
  `BlueprintNativeEvent`는 선언과 구현의 이름이 달라서 한쪽만 찾으면 호출 사슬이 끊긴다.
