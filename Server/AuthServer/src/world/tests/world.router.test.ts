import express from 'express';
import request from 'supertest';
import { afterEach, beforeEach, describe, expect, test, vi } from 'vitest';

import { errorHandler, notFoundHandler } from '../../common/errorHandlers.js';
import { MemoryLoginSessionRepository } from '../../common/loginSession.memory.js';
import { MemoryTicketRepository } from '../../common/ticket.memory.js';
import type { TicketRepository } from '../../common/ticket.js';
import { MemoryWorldDirectory } from '../../common/worldDirectory.memory.js';
import { MemoryAccountStatusRepository } from '../world.repository.memory.js';
import { createWorldRouter } from '../world.router.js';
import { WorldService } from '../world.service.js';

// world 라우터와 공용 핸들러를 app.ts와 같은 순서로 메모리 저장소와 조립해 HTTP 계약을 검사한다.
// 로그인 세션은 로그인을 거치지 않고 저장소에 직접 넣는다. 다른 도메인의 라우터는 붙이지 않는다.

let loginSessions: MemoryLoginSessionRepository;
let worlds: MemoryWorldDirectory;
let accounts: MemoryAccountStatusRepository;
let tickets: MemoryTicketRepository;

function appWith(ticketRepository: TicketRepository = tickets) {
    const app = express();
    app.use(express.json());
    app.use('/api/worlds', createWorldRouter(new WorldService(loginSessions, worlds, accounts, ticketRepository)));
    app.use(notFoundHandler);
    app.use(errorHandler);
    return app;
}

const AUTH = { Authorization: 'Bearer session-42' };

beforeEach(async () => {
    loginSessions = new MemoryLoginSessionRepository();
    worlds = new MemoryWorldDirectory();
    accounts = new MemoryAccountStatusRepository();
    tickets = new MemoryTicketRepository();

    worlds.set({ worldId: 1, name: '하나', host: '10.0.0.1', port: 7777, congestion: 'NORMAL' });
    accounts.set('42', null);
    await loginSessions.save('session-42', '42');
});

afterEach(() => {
    vi.restoreAllMocks();
});

describe('GET /api/worlds', () => {
    test('성공하면 200과 월드 목록을 성공 봉투에 담는다', async () => {
        const res = await request(appWith()).get('/api/worlds').set(AUTH);

        expect(res.status).toBe(200);
        expect(res.body).toEqual({ ok: true, data: { worlds: [{ worldId: 1, name: '하나', congestion: 'NORMAL' }] } });
    });

    test.each([
        ['헤더 없음', undefined],
        ['Bearer가 아님', 'Basic session-42'],
        ['값이 비었음', 'Bearer '],
        ['없는 로그인 세션', 'Bearer nobody'],
    ])('%s이면 401과 LOGIN_SESSION_EXPIRED다', async (_case, header) => {
        const req = request(appWith()).get('/api/worlds');
        const res = await (header === undefined ? req : req.set('Authorization', header));

        expect(res.status).toBe(401);
        expect(res.body).toEqual({ ok: false, error: { code: 'LOGIN_SESSION_EXPIRED', message: expect.any(String) } });
    });
});

describe('POST /api/worlds/:worldId/select', () => {
    test('성공하면 200과 그 월드의 주소, 그 월드에 묶인 인증 티켓을 성공 봉투에 담는다', async () => {
        const res = await request(appWith()).post('/api/worlds/1/select').set(AUTH);

        expect(res.status).toBe(200);
        expect(res.body).toEqual({ ok: true, data: { host: '10.0.0.1', port: 7777, ticket: expect.any(String) } });
        expect(tickets.accountIdOf(1, res.body.data.ticket)).toBe('42');
    });

    test('없는 로그인 세션이면 401과 LOGIN_SESSION_EXPIRED이고 티켓을 발급하지 않는다', async () => {
        const res = await request(appWith()).post('/api/worlds/1/select').set('Authorization', 'Bearer nobody');

        expect(res.status).toBe(401);
        expect(res.body.error.code).toBe('LOGIN_SESSION_EXPIRED');
        expect(tickets.count()).toBe(0);
    });

    test('항목이 사라진 월드면 409와 WORLD_UNAVAILABLE이다', async () => {
        worlds.remove(1);

        const res = await request(appWith()).post('/api/worlds/1/select').set(AUTH);

        expect(res.status).toBe(409);
        expect(res.body).toEqual({ ok: false, error: { code: 'WORLD_UNAVAILABLE', message: expect.any(String) } });
        expect(tickets.count()).toBe(0);
    });

    test('정지된 계정이면 403과 ACCOUNT_BANNED이고 정지 기한을 error에 싣는다', async () => {
        const bannedUntil = new Date(Date.now() + 60 * 60 * 1000);
        accounts.set('42', bannedUntil);

        const res = await request(appWith()).post('/api/worlds/1/select').set(AUTH);

        expect(res.status).toBe(403);
        expect(res.body).toEqual({
            ok: false,
            error: { code: 'ACCOUNT_BANNED', message: expect.any(String), bannedUntil: bannedUntil.toISOString() },
        });
        expect(tickets.count()).toBe(0);
    });

    test.each(['0', '01', '-1', 'abc', '1.5', '2147483648'])('월드 번호가 %s이면 400과 VALIDATION_FAILED다', async (worldId) => {
        const res = await request(appWith()).post(`/api/worlds/${worldId}/select`).set(AUTH);

        expect(res.status).toBe(400);
        expect(res.body.error.code).toBe('VALIDATION_FAILED');
        expect(tickets.count()).toBe(0);
    });

    test('로그인 세션이 없으면 월드 번호가 틀려도 401이다', async () => {
        const res = await request(appWith()).post('/api/worlds/abc/select');

        expect(res.status).toBe(401);
    });

    test('티켓 저장소가 실패하면 500과 INTERNAL_ERROR이고, 에러 원문은 응답에 싣지 않는다', async () => {
        const failing: TicketRepository = { save: () => Promise.reject(new Error('Redis 접속 끊김')) };
        vi.spyOn(console, 'error').mockImplementation(() => {});

        const res = await request(appWith(failing)).post('/api/worlds/1/select').set(AUTH);

        expect(res.status).toBe(500);
        expect(res.body).toEqual({ ok: false, error: { code: 'INTERNAL_ERROR', message: expect.any(String) } });
        expect(JSON.stringify(res.body)).not.toContain('Redis 접속 끊김');
    });
});
