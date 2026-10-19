-- 001: 초기 스키마 (계정).
-- 이미 적용된 DB에 다시 실행해도 아무것도 바꾸지 않는다. 테이블이 없을 때만 만든다.
-- 스키마를 바꿀 때는 이 파일을 고치지 않고 다음 번호의 파일(002_<이름>.sql)을 더한다.

-- 계정 번호 발급과 로그인 판정
IF OBJECT_ID(N'dbo.Account', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Account
    (
        AccountId    BIGINT       IDENTITY(1, 1) NOT NULL,
        LoginId      VARCHAR(32)  NOT NULL, -- 소문자로 바꿔서 저장한다
        PasswordHash VARCHAR(255) NOT NULL, -- 솔트가 들어 있는 bcrypt 결과 문자열
        BannedUntil  DATETIME2(3) NULL,     -- NULL이면 정상, 값이 있으면 그 시각(UTC)까지 로그인을 거절한다
        CreatedAt    DATETIME2(3) NOT NULL
            CONSTRAINT DF_Account_CreatedAt DEFAULT SYSUTCDATETIME(),
        LastLoginAt  DATETIME2(3) NULL,     -- UTC
        CONSTRAINT PK_Account PRIMARY KEY (AccountId),
        CONSTRAINT UQ_Account_LoginId UNIQUE (LoginId)
    );
END;
