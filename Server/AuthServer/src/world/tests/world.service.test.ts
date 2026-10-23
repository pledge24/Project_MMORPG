import { beforeEach, describe, expect, test } from 'vitest';

import { AppError } from '../../common/errors.js';
import { LOGIN_SESSION_TTL_SECONDS } from '../../common/loginSession.js';
import { MemoryLoginSessionRepository } from '../../common/loginSession.memory.js';
import { MemoryTicketRepository } from '../../common/ticket.memory.js';
import { MemoryWorldDirectory } from '../../common/worldDirectory.memory.js';
import { MemoryAccountStatusRepository } from '../world.repository.memory.js';
import { WorldService } from '../world.service.js';

// 로그인 세션은 로그인을 거치지 않고 저장소에 직접 넣는다. 로그인이 로그인 세션을 만드는 것은 auth의 테스트가 본다.

const LOGIN_AT = new Date('2026-10-23T12:00:00.000Z');
const UUID_PATTERN = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;

const WORLD_1 = { worldId: 1, name: '하나', host: '10.0.0.1', port: 7777, congestion: 'SMOOTH' } as const;
const WORLD_2 = { worldId: 2, name: '둘', host: '10.0.0.2', port: 7778, congestion: 'CROWDED' } as const;

let now: Date;
let loginSessions: MemoryLoginSessionRepository;
let worlds: MemoryWorldDirectory;
let accounts: MemoryAccountStatusRepository;
let tickets: MemoryTicketRepository;
let service: WorldService;

beforeEach(async () => {
    now = LOGIN_AT;
    loginSessions = new MemoryLoginSessionRepository(() => now);
    worlds = new MemoryWorldDirectory();
    accounts = new MemoryAccountStatusRepository();
    tickets = new MemoryTicketRepository();
    service = new WorldService(loginSessions, worlds, accounts, tickets, () => now);

    worlds.set(WORLD_2);
    worlds.set(WORLD_1);
    accounts.set('42', null);
    await loginSessions.save('session-42', '42');
});

async function errorOf(promise: Promise<unknown>): Promise<AppError> {
    const err = await promise.then(() => undefined, (e: unknown) => e);
    expect(err).toBeInstanceOf(AppError);
    return err as AppError;
}

describe('WorldService.listWorlds', () => {
    test('살아 있는 월드를 월드 번호 순으로 돌려주고 접속 주소는 싣지 않는다', async () => {
        const result = await service.listWorlds('session-42');

        expect(result).toEqual({
            worlds: [
                { worldId: 1, name: '하나', congestion: 'SMOOTH' },
                { worldId: 2, name: '둘', congestion: 'CROWDED' },
            ],
        });
    });

    test('항목이 만료된 월드는 목록에서 빠진다', async () => {
        worlds.remove(2);

        const result = await service.listWorlds('session-42');

        expect(result.worlds.map((world) => world.worldId)).toEqual([1]);
    });

    test('없는 로그인 세션이면 LOGIN_SESSION_EXPIRED다', async () => {
        expect((await errorOf(service.listWorlds('nobody'))).code).toBe('LOGIN_SESSION_EXPIRED');
    });

    test('로그인한 지 1시간이 지난 로그인 세션이면 LOGIN_SESSION_EXPIRED다', async () => {
        now = new Date(LOGIN_AT.getTime() + LOGIN_SESSION_TTL_SECONDS * 1000);

        expect((await errorOf(service.listWorlds('session-42'))).code).toBe('LOGIN_SESSION_EXPIRED');
    });
});

describe('WorldService.selectWorld', () => {
    test('그 월드의 주소와 그 월드에 묶인 인증 티켓을 돌려준다', async () => {
        const result = await service.selectWorld('session-42', 2);

        expect(result).toEqual({ host: '10.0.0.2', port: 7778, ticket: expect.stringMatching(UUID_PATTERN) });
        expect(tickets.accountIdOf(2, result.ticket)).toBe('42');
        expect(tickets.accountIdOf(1, result.ticket)).toBeUndefined();
    });

    test('혼잡한 월드도 고를 수 있다', async () => {
        await expect(service.selectWorld('session-42', 2)).resolves.toHaveProperty('ticket');
    });

    test('월드를 골라도 로그인 세션은 남고 수명도 늘어나지 않는다', async () => {
        now = new Date(LOGIN_AT.getTime() + 30 * 60 * 1000);
        await service.selectWorld('session-42', 1);

        await expect(service.listWorlds('session-42')).resolves.toBeDefined();

        now = new Date(LOGIN_AT.getTime() + LOGIN_SESSION_TTL_SECONDS * 1000);
        expect((await errorOf(service.listWorlds('session-42'))).code).toBe('LOGIN_SESSION_EXPIRED');
    });

    test('같은 월드를 다시 고르면 새 티켓을 발급한다', async () => {
        const first = await service.selectWorld('session-42', 1);
        const second = await service.selectWorld('session-42', 1);

        expect(second.ticket).not.toBe(first.ticket);
        expect(tickets.count()).toBe(2);
    });

    test('만료된 로그인 세션이면 LOGIN_SESSION_EXPIRED이고 티켓을 발급하지 않는다', async () => {
        now = new Date(LOGIN_AT.getTime() + LOGIN_SESSION_TTL_SECONDS * 1000);

        expect((await errorOf(service.selectWorld('session-42', 1))).code).toBe('LOGIN_SESSION_EXPIRED');
        expect(tickets.count()).toBe(0);
    });

    test('항목이 사라진 월드면 WORLD_UNAVAILABLE이고 티켓을 발급하지 않는다', async () => {
        worlds.remove(1);

        expect((await errorOf(service.selectWorld('session-42', 1))).code).toBe('WORLD_UNAVAILABLE');
        expect(tickets.count()).toBe(0);
    });

    test('로그인한 뒤 정지된 계정이면 ACCOUNT_BANNED와 정지 기한을 주고 티켓을 발급하지 않는다', async () => {
        accounts.set('42', new Date('2026-10-24T00:00:00.000Z'));

        const err = await errorOf(service.selectWorld('session-42', 1));

        expect(err.code).toBe('ACCOUNT_BANNED');
        expect(err.details).toEqual({ bannedUntil: '2026-10-24T00:00:00.000Z' });
        expect(tickets.count()).toBe(0);
    });

    test('정지 기한이 지금과 같으면 고를 수 있다', async () => {
        accounts.set('42', LOGIN_AT);

        await expect(service.selectWorld('session-42', 1)).resolves.toHaveProperty('ticket');
    });

    test('로그인 세션의 계정이 없으면 LOGIN_SESSION_EXPIRED다', async () => {
        await loginSessions.save('session-ghost', '999');

        expect((await errorOf(service.selectWorld('session-ghost', 1))).code).toBe('LOGIN_SESSION_EXPIRED');
    });
});
