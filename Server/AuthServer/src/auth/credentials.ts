import { AppError } from '../errors.js';

export interface Credentials {
    /** 소문자로 바꾼 값 */
    loginId: string;
    password: string;
}

// 로그인 ID는 소문자로 바꾼 뒤에 검사하므로 대문자를 받지 않는다.
const LOGIN_ID_PATTERN = /^[a-z0-9]{4,10}$/;
const PASSWORD_PATTERN = /^[A-Za-z0-9]{4,20}$/;

/**
 * 요청 본문에서 로그인 ID와 비밀번호를 꺼내 로그인 ID를 소문자로 바꾸고 형식을 검사한다.
 * 형식에 맞지 않으면 VALIDATION_FAILED를 던진다.
 */
export function parseCredentials(body: unknown): Credentials {
    if (typeof body !== 'object' || body === null) {
        throw new AppError('VALIDATION_FAILED');
    }

    const { loginId, password } = body as Record<string, unknown>;

    const normalizedLoginId = typeof loginId === 'string' ? loginId.toLowerCase() : '';
    if (!LOGIN_ID_PATTERN.test(normalizedLoginId)) {
        throw new AppError('VALIDATION_FAILED', '로그인 ID는 영문과 숫자로 된 4~10자여야 합니다.');
    }

    if (typeof password !== 'string' || !PASSWORD_PATTERN.test(password)) {
        throw new AppError('VALIDATION_FAILED', '비밀번호는 영문과 숫자로 된 4~20자여야 합니다.');
    }

    return { loginId: normalizedLoginId, password };
}
