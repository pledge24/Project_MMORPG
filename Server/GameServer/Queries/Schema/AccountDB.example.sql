-- 계정 정보: AccountId 발급과 로그인 인증
CREATE TABLE Account
(
    AccountId    BIGINT       IDENTITY(1, 1) NOT NULL,
    LoginId      VARCHAR(32)  NOT NULL, -- 소문자로 정규화하여 저장
    PasswordHash VARCHAR(255) NOT NULL, -- bcrypt 결과 문자열 (솔트 포함)
    GmLevel      TINYINT      NOT NULL
        CONSTRAINT DF_Account_GmLevel DEFAULT 0, -- 0:일반 사용자
    BannedUntil  DATETIME2(3) NULL, -- NULL이면 정상, 값이 있으면 해당 시각까지 로그인 금지
    CreatedAt    DATETIME2(3) NOT NULL
        CONSTRAINT DF_Account_CreatedAt DEFAULT SYSUTCDATETIME(),
    LastLoginAt  DATETIME2(3) NULL,
    CONSTRAINT PK_Account PRIMARY KEY (AccountId),
    CONSTRAINT UQ_Account_LoginId UNIQUE (LoginId)
);