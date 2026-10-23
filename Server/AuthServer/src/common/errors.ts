/**
 * 클라이언트에 돌려줄 실패의 코드 카탈로그. 코드마다 HTTP 상태와 기본 문구가 정해져 있다.
 * 문구는 클라이언트가 화면에 그대로 보여 준다. 클라이언트는 문구가 아니라 코드로 분기한다.
 */
export const ERROR_CATALOG = {
    VALIDATION_FAILED: { status: 400, message: '요청 형식이 올바르지 않습니다.' },
    INVALID_CREDENTIALS: { status: 401, message: '로그인 ID 또는 비밀번호가 올바르지 않습니다.' },
    LOGIN_SESSION_EXPIRED: { status: 401, message: '로그인이 만료되었습니다. 다시 로그인해 주세요.' },
    ACCOUNT_BANNED: { status: 403, message: '이용이 정지된 계정입니다.' },
    NOT_FOUND: { status: 404, message: '요청한 경로가 없습니다.' },
    DUPLICATE_LOGIN_ID: { status: 409, message: '이미 사용 중인 로그인 ID입니다.' },
    WORLD_UNAVAILABLE: { status: 409, message: '지금은 이 월드에 접속할 수 없습니다. 월드 목록을 다시 확인해 주세요.' },
    INTERNAL_ERROR: { status: 500, message: '서버에서 오류가 발생했습니다. 잠시 후 다시 시도해 주세요.' },
} as const satisfies Record<string, { status: number; message: string }>;

export type ErrorCode = keyof typeof ERROR_CATALOG;

/** 코드와 문구 말고 실패 봉투의 error에 함께 싣는 값 */
export interface ErrorDetails {
    /** ACCOUNT_BANNED의 정지 해제 시각. UTC 기준 ISO 8601 형식이다. */
    bannedUntil?: string;
}

/** 클라이언트에 코드와 문구로 알릴 실패. 이 밖의 예외는 모두 INTERNAL_ERROR로 응답한다. */
export class AppError extends Error {
    readonly code: ErrorCode;
    readonly status: number;
    readonly details: ErrorDetails;

    constructor(code: ErrorCode, message: string = ERROR_CATALOG[code].message, details: ErrorDetails = {}) {
        super(message);
        this.name = 'AppError';
        this.code = code;
        this.status = ERROR_CATALOG[code].status;
        this.details = details;
    }
}
