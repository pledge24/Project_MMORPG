import express from 'express';
import request from 'supertest';
import { expect, test } from 'vitest';

import { errorHandler, notFoundHandler } from './errorHandlers.js';

// 도메인 라우터 없이 공용 핸들러만 붙인 앱. 도메인 폴더를 지워도 이 테스트는 남는다.
function bareApp() {
    const app = express();
    app.use(notFoundHandler);
    app.use(errorHandler);
    return app;
}

test('없는 경로는 404와 NOT_FOUND다', async () => {
    const res = await request(bareApp()).get('/api/nothing');

    expect(res.status).toBe(404);
    expect(res.body).toEqual({ ok: false, error: { code: 'NOT_FOUND', message: expect.any(String) } });
});
