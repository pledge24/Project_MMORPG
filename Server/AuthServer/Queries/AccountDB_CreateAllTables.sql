-- AccountDB에서 사용하는 모든 테이블
-- 보존할 계정 데이터가 없으므로 스키마를 바꿀 때는 이 스크립트를 다시 실행해 테이블을 새로 만든다.
DROP TABLE IF EXISTS Account;

-- 1. Account: 계정 번호 발급과 로그인 판정
CREATE TABLE Account
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
