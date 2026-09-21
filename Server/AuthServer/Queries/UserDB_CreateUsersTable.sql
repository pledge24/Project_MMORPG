-- UserDB에서 사용하는 모든 테이블
DROP TABLE IF EXISTS Users;

-- 1. Users: 유저 계정 정보
CREATE TABLE Users
(
	user_id			INT IDENTITY(1, 1) PRIMARY KEY,
	username		NVARCHAR(50) NOT NULL UNIQUE,
	password_hash	NVARCHAR(255) NOT NULL,
	created_at		DATETIME2 DEFAULT GETDATE(),
	last_login		DATETIME2 NULL
)

--SELECT @@VERSION;