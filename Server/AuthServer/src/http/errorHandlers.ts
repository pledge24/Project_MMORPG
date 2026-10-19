import type { ErrorRequestHandler, RequestHandler } from 'express';

import { AppError } from '../errors.js';
import { failure } from './envelope.js';

/** 어느 라우터도 받지 않은 요청을 NOT_FOUND로 넘긴다. 라우터를 모두 붙인 뒤에 붙인다. */
export const notFoundHandler: RequestHandler = (_req, _res, next) => {
    next(new AppError('NOT_FOUND'));
};

/** 모든 실패를 실패 봉투로 바꿔 응답한다. 앱에 마지막으로 붙인다. */
export const errorHandler: ErrorRequestHandler = (err, _req, res, _next) => {
    const appError = toAppError(err);

    // 예상하지 못한 실패는 응답에 원문을 싣지 않으므로 여기서 남긴다. 없으면 bcrypt, MSSQL, Redis 중 무엇이 죽었는지 모른다.
    if (appError.code === 'INTERNAL_ERROR') {
        console.error('요청 처리 중 오류:', err);
    }

    res.status(appError.status).json(failure(appError.code, appError.message, appError.details));
};

function toAppError(err: unknown): AppError {
    if (err instanceof AppError) {
        return err;
    }
    // express.json()이 본문을 JSON으로 읽지 못하면 이 type을 단 에러를 넘긴다.
    if (typeof err === 'object' && err !== null && (err as { type?: unknown }).type === 'entity.parse.failed') {
        return new AppError('VALIDATION_FAILED', '요청 본문이 JSON 형식이 아닙니다.');
    }
    return new AppError('INTERNAL_ERROR');
}
