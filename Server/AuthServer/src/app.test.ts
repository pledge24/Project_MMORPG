import request from 'supertest';
import { afterEach, describe, expect, test, vi } from 'vitest';

import type { AccountRepository } from './accounts/accountRepository.js';
import { MemoryAccountRepository } from './accounts/memoryAccountRepository.js';
import { createApp } from './app.js';
import { RegisterService } from './auth/registerService.js';

function appWith(accounts: AccountRepository = new MemoryAccountRepository()) {
    return createApp({ registerService: new RegisterService(accounts) });
}

afterEach(() => {
    vi.restoreAllMocks();
});

describe('POST /api/auth/register', () => {
    test('성공하면 201과 소문자로 바꾼 로그인 ID를 성공 봉투에 담는다', async () => {
        const res = await request(appWith()).post('/api/auth/register').send({ loginId: 'Player01', password: 'pass1234' });

        expect(res.status).toBe(201);
        expect(res.body).toEqual({ ok: true, data: { loginId: 'player01' } });
    });

    test('같은 로그인 ID면 409와 DUPLICATE_LOGIN_ID다', async () => {
        const app = appWith();
        await request(app).post('/api/auth/register').send({ loginId: 'player01', password: 'pass1234' });

        const res = await request(app).post('/api/auth/register').send({ loginId: 'PLAYER01', password: 'pass1234' });

        expect(res.status).toBe(409);
        expect(res.body).toEqual({ ok: false, error: { code: 'DUPLICATE_LOGIN_ID', message: expect.any(String) } });
    });

    test('형식이 틀리면 400과 VALIDATION_FAILED다', async () => {
        const res = await request(appWith()).post('/api/auth/register').send({ loginId: 'ab', password: 'pass1234' });

        expect(res.status).toBe(400);
        expect(res.body).toEqual({ ok: false, error: { code: 'VALIDATION_FAILED', message: expect.any(String) } });
    });

    test('본문이 JSON으로 읽히지 않으면 400과 VALIDATION_FAILED다', async () => {
        const res = await request(appWith())
            .post('/api/auth/register')
            .set('Content-Type', 'application/json')
            .send('{"loginId":');

        expect(res.status).toBe(400);
        expect(res.body.error.code).toBe('VALIDATION_FAILED');
    });

    test('본문이 없으면 400과 VALIDATION_FAILED다', async () => {
        const res = await request(appWith()).post('/api/auth/register');

        expect(res.status).toBe(400);
        expect(res.body.error.code).toBe('VALIDATION_FAILED');
    });

    test('저장소가 실패하면 500과 INTERNAL_ERROR이고, 에러 원문은 응답에 싣지 않는다', async () => {
        const failing: AccountRepository = { create: () => Promise.reject(new Error('DB 접속 끊김')) };
        vi.spyOn(console, 'error').mockImplementation(() => {});

        const res = await request(appWith(failing)).post('/api/auth/register').send({ loginId: 'player01', password: 'pass1234' });

        expect(res.status).toBe(500);
        expect(res.body).toEqual({ ok: false, error: { code: 'INTERNAL_ERROR', message: expect.any(String) } });
        expect(JSON.stringify(res.body)).not.toContain('DB 접속 끊김');
        expect(console.error).toHaveBeenCalled();
    });
});

test('없는 경로는 404와 NOT_FOUND다', async () => {
    const res = await request(appWith()).get('/api/auth/nothing');

    expect(res.status).toBe(404);
    expect(res.body).toEqual({ ok: false, error: { code: 'NOT_FOUND', message: expect.any(String) } });
});
