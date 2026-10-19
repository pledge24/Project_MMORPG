import request from 'supertest';
import { afterEach, beforeEach, describe, expect, test, vi } from 'vitest';

import { createApp } from '../app.js';
import { MemoryTicketRepository } from '../common/ticket.memory.js';
import type { TicketRepository } from '../common/ticket.js';
import type { AccountRepository } from './auth.repository.js';
import { MemoryAccountRepository } from './auth.repository.memory.js';
import { AuthService } from './auth.service.js';

// 앱 전체를 메모리 저장소로 조립해 HTTP 계약(상태 코드와 봉투)을 검사한다. 본문 형식 검사도 여기서 검사한다.

function appWith(accounts: AccountRepository = new MemoryAccountRepository(), tickets: TicketRepository = new MemoryTicketRepository()) {
    return createApp({ authService: new AuthService(accounts, tickets) });
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
    let tickets: MemoryTicketRepository;
    let app: ReturnType<typeof appWith>;

    beforeEach(async () => {
        accounts = new MemoryAccountRepository();
        tickets = new MemoryTicketRepository();
        app = appWith(accounts, tickets);
        await request(app).post('/api/auth/register').send({ loginId: 'player01', password: 'pass1234' });
    });

    test('성공하면 200과 티켓을 성공 봉투에 담는다', async () => {
        const res = await request(app).post('/api/auth/login').send({ loginId: 'Player01', password: 'pass1234' });

        expect(res.status).toBe(200);
        expect(res.body).toEqual({ ok: true, data: { ticket: expect.any(String) } });
        expect(tickets.accountIdOf(res.body.data.ticket)).toBe(accounts.get('player01')!.accountId);
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
        expect(tickets.count()).toBe(0);
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

    test('티켓 저장소가 실패하면 500과 INTERNAL_ERROR이고, 에러 원문은 응답에 싣지 않는다', async () => {
        const failing: TicketRepository = { save: () => Promise.reject(new Error('Redis 접속 끊김')) };
        vi.spyOn(console, 'error').mockImplementation(() => {});

        const res = await request(appWith(accounts, failing)).post('/api/auth/login').send({ loginId: 'player01', password: 'pass1234' });

        expect(res.status).toBe(500);
        expect(res.body).toEqual({ ok: false, error: { code: 'INTERNAL_ERROR', message: expect.any(String) } });
        expect(JSON.stringify(res.body)).not.toContain('Redis 접속 끊김');
        expect(console.error).toHaveBeenCalled();
    });

    test('옛 로그인 경로는 404다', async () => {
        const res = await request(app).post('/Login').send({ username: 'player01', password: 'pass1234' });

        expect(res.status).toBe(404);
    });
});

// 형식 규칙: 로그인 ID는 소문자로 바꾼 뒤 영문 소문자와 숫자 4~10자, 비밀번호는 영문과 숫자 4~20자다.
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

    test.each(MALFORMED_BODIES)('로그인: %s이면 400과 VALIDATION_FAILED이고 티켓을 발급하지 않는다', async (_case, body) => {
        const tickets = new MemoryTicketRepository();

        const res = await request(appWith(undefined, tickets)).post('/api/auth/login').set('Content-Type', 'application/json').send(JSON.stringify(body));

        expect(res.status).toBe(400);
        expect(res.body.error.code).toBe('VALIDATION_FAILED');
        expect(tickets.count()).toBe(0);
    });
});
