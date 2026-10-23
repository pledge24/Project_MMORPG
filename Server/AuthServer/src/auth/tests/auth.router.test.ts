import express from 'express';
import request from 'supertest';
import { afterEach, beforeEach, describe, expect, test, vi } from 'vitest';

import { errorHandler, notFoundHandler } from '../../common/errorHandlers.js';
import type { LoginSessionRepository } from '../../common/loginSession.js';
import { MemoryLoginSessionRepository } from '../../common/loginSession.memory.js';
import { MemoryWorldDirectory } from '../../common/worldDirectory.memory.js';
import type { AccountRepository } from '../auth.repository.js';
import { MemoryAccountRepository } from '../auth.repository.memory.js';
import { createAuthRouter } from '../auth.router.js';
import { AuthService } from '../auth.service.js';

// auth 라우터와 공용 핸들러를 app.ts와 같은 순서로 메모리 저장소와 조립해 HTTP 계약(상태 코드와 봉투)을 검사한다.
// 본문 형식이 틀린 경우도 여기서 다룬다. 다른 도메인의 라우터는 붙이지 않는다. 도메인을 지우면 그 테스트만 사라진다.

function appWith(
    accounts: AccountRepository = new MemoryAccountRepository(),
    loginSessions: LoginSessionRepository = new MemoryLoginSessionRepository(),
    worlds = new MemoryWorldDirectory(),
) {
    const app = express();
    app.use(express.json());
    app.use('/api/auth', createAuthRouter(new AuthService(accounts, loginSessions, worlds)));
    app.use(notFoundHandler);
    app.use(errorHandler);
    return app;
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
        const dbDown = () => Promise.reject(new Error('DB 접속 끊김'));
        const failing: AccountRepository = { create: dbDown, findByLoginId: dbDown, recordLogin: dbDown };
        vi.spyOn(console, 'error').mockImplementation(() => {});

        const res = await request(appWith(failing)).post('/api/auth/register').send({ loginId: 'player01', password: 'pass1234' });

        expect(res.status).toBe(500);
        expect(res.body).toEqual({ ok: false, error: { code: 'INTERNAL_ERROR', message: expect.any(String) } });
        expect(JSON.stringify(res.body)).not.toContain('DB 접속 끊김');
        expect(console.error).toHaveBeenCalled();
    });
});

describe('POST /api/auth/login', () => {
    let accounts: MemoryAccountRepository;
    let loginSessions: MemoryLoginSessionRepository;
    let worlds: MemoryWorldDirectory;
    let app: ReturnType<typeof appWith>;

    beforeEach(async () => {
        accounts = new MemoryAccountRepository();
        loginSessions = new MemoryLoginSessionRepository();
        worlds = new MemoryWorldDirectory();
        app = appWith(accounts, loginSessions, worlds);
        await request(app).post('/api/auth/register').send({ loginId: 'player01', password: 'pass1234' });
    });

    test('성공하면 200과 로그인 세션, 월드 목록을 성공 봉투에 담는다', async () => {
        worlds.set({ worldId: 1, name: '하나', host: '10.0.0.1', port: 7777, congestion: 'CROWDED' });

        const res = await request(app).post('/api/auth/login').send({ loginId: 'Player01', password: 'pass1234' });

        expect(res.status).toBe(200);
        expect(res.body).toEqual({
            ok: true,
            data: { loginSession: expect.any(String), worlds: [{ worldId: 1, name: '하나', congestion: 'CROWDED' }] },
        });
        expect(await loginSessions.findAccountId(res.body.data.loginSession)).toBe(accounts.get('player01')!.accountId);
    });


    test('없는 로그인 ID와 틀린 비밀번호는 같은 401과 INVALID_CREDENTIALS다', async () => {
        const unknownId = await request(app).post('/api/auth/login').send({ loginId: 'nobody', password: 'pass1234' });
        const wrongPassword = await request(app).post('/api/auth/login').send({ loginId: 'player01', password: 'wrong999' });

        expect(unknownId.status).toBe(401);
        expect(unknownId.body).toEqual({ ok: false, error: { code: 'INVALID_CREDENTIALS', message: expect.any(String) } });
        expect(wrongPassword.status).toBe(unknownId.status);
        expect(wrongPassword.body).toEqual(unknownId.body);
    });

    test('정지 중이면 403과 ACCOUNT_BANNED이고 정지 기한을 error에 싣는다', async () => {
        const bannedUntil = new Date(Date.now() + 60 * 60 * 1000);
        accounts.setBannedUntil('player01', bannedUntil);

        const res = await request(app).post('/api/auth/login').send({ loginId: 'player01', password: 'pass1234' });

        expect(res.status).toBe(403);
        expect(res.body).toEqual({
            ok: false,
            error: { code: 'ACCOUNT_BANNED', message: expect.any(String), bannedUntil: bannedUntil.toISOString() },
        });
        expect(loginSessions.count()).toBe(0);
    });

    test('형식이 틀리면 400과 VALIDATION_FAILED다', async () => {
        const res = await request(app).post('/api/auth/login').send({ loginId: 'player01', password: 'a' });

        expect(res.status).toBe(400);
        expect(res.body).toEqual({ ok: false, error: { code: 'VALIDATION_FAILED', message: expect.any(String) } });
    });

    test('본문이 없으면 400과 VALIDATION_FAILED다', async () => {
        const res = await request(app).post('/api/auth/login');

        expect(res.status).toBe(400);
        expect(res.body.error.code).toBe('VALIDATION_FAILED');
    });

    test('로그인 세션 저장소가 실패하면 500과 INTERNAL_ERROR이고, 에러 원문은 응답에 싣지 않는다', async () => {
        const redisDown = () => Promise.reject(new Error('Redis 접속 끊김'));
        const failing: LoginSessionRepository = { save: redisDown, findAccountId: redisDown };
        vi.spyOn(console, 'error').mockImplementation(() => {});

        const res = await request(appWith(accounts, failing)).post('/api/auth/login').send({ loginId: 'player01', password: 'pass1234' });

        expect(res.status).toBe(500);
        expect(res.body).toEqual({ ok: false, error: { code: 'INTERNAL_ERROR', message: expect.any(String) } });
        expect(JSON.stringify(res.body)).not.toContain('Redis 접속 끊김');
        expect(console.error).toHaveBeenCalled();
    });

    test('옛 로그인 경로는 404와 NOT_FOUND 봉투다', async () => {
        const res = await request(app).post('/Login').send({ username: 'player01', password: 'pass1234' });

        expect(res.status).toBe(404);
        expect(res.body).toEqual({ ok: false, error: { code: 'NOT_FOUND', message: expect.any(String) } });
    });
});

// 형식 규칙은 auth.handler.ts의 정규식이 정한다. 아래는 그 규칙의 경계를 하나씩 넘는 본문이다.
const MALFORMED_BODIES = [
    ['본문이 객체가 아님', 'player01'],
    ['로그인 ID 없음', { password: 'pass1234' }],
    ['로그인 ID가 문자열이 아님', { loginId: 1234, password: 'pass1234' }],
    ['로그인 ID가 4자 미만', { loginId: 'abc', password: 'pass1234' }],
    ['로그인 ID가 10자 초과', { loginId: 'abcdefghijk', password: 'pass1234' }],
    ['로그인 ID에 특수문자', { loginId: 'play_er', password: 'pass1234' }],
    ['비밀번호 없음', { loginId: 'player01' }],
    ['비밀번호가 4자 미만', { loginId: 'player01', password: 'abc' }],
    ['비밀번호가 20자 초과', { loginId: 'player01', password: 'a'.repeat(21) }],
    ['비밀번호에 특수문자', { loginId: 'player01', password: 'pass!234' }],
] as const;

describe('본문 형식 검사', () => {
    test.each(MALFORMED_BODIES)('회원가입: %s이면 400과 VALIDATION_FAILED이고 계정을 만들지 않는다', async (_case, body) => {
        const accounts = new MemoryAccountRepository();

        const res = await request(appWith(accounts)).post('/api/auth/register').set('Content-Type', 'application/json').send(JSON.stringify(body));

        expect(res.status).toBe(400);
        expect(res.body.error.code).toBe('VALIDATION_FAILED');
        expect(accounts.count()).toBe(0);
    });

    test.each(MALFORMED_BODIES)('로그인: %s이면 400과 VALIDATION_FAILED이고 로그인 세션을 만들지 않는다', async (_case, body) => {
        const loginSessions = new MemoryLoginSessionRepository();

        const res = await request(appWith(undefined, loginSessions)).post('/api/auth/login').set('Content-Type', 'application/json').send(JSON.stringify(body));

        expect(res.status).toBe(400);
        expect(res.body.error.code).toBe('VALIDATION_FAILED');
        expect(loginSessions.count()).toBe(0);
    });
});
