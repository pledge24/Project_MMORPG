import type { ErrorCode, ErrorDetails } from '../errors.js';

// 모든 응답 본문은 이 두 형태 중 하나다. 클라이언트는 상태 코드가 아니라 ok로 성공과 실패를 가른다.
export interface SuccessBody<T> {
    ok: true;
    data: T;
}

export interface FailureBody {
    ok: false;
    error: { code: ErrorCode; message: string } & ErrorDetails;
}

export function success<T>(data: T): SuccessBody<T> {
    return { ok: true, data };
}

export function failure(code: ErrorCode, message: string, details: ErrorDetails = {}): FailureBody {
    return { ok: false, error: { code, message, ...details } };
}
