import bcrypt from 'bcrypt';
import { beforeEach, describe, expect, test } from 'vitest';

import { AppError } from '../../common/errors.js';
import { MemoryTicketRepository } from '../../common/ticket.memory.js';
import { MemoryAccountRepository } from '../auth.repository.memory.js';
import { AuthService } from '../auth.service.js';

// 형식 검사와 로그인 ID 소문자 변환은 handler가 하므로, 여기서는 형식을 통과한 값만 넣는다. 형식 실패는 HTTP 테스트가 검사한다.

const NOW = new Date('2026-10-19T12:00:00.000Z');
const UUID_PATTERN = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;

let accounts: MemoryAccountRepository;
let tickets: MemoryTicketRepository;
let service: AuthService;

beforeEach(() => {
    accounts = new MemoryAccountRepository();
    tickets = new MemoryTicketRepository();
    service = new AuthService(accounts, tickets, () => NOW);
});

async function errorOf(promise: Promise<unknown>): Promise<AppError> {
    const err = await promise.then(() => undefined, (e: unknown) => e);
    expect(err).toBeInstanceOf(AppError);
    return err as AppError;
}

describe('AuthService.register', () => {
    test('계정을 만들고 로그인 ID를 돌려준다. 비밀번호는 bcrypt 해시로 저장한다', async () => {
        const result = await service.register({ loginId: 'player01', password: 'pass1234' });

        expect(result).toEqual({ loginId: 'player01' });
        const stored = accounts.get('player01');
        expect(stored).toBeDefined();
        expect(await bcrypt.compare('pass1234', stored!.passwordHash)).toBe(true);
    });

    test('같은 로그인 ID로 다시 가입하면 DUPLICATE_LOGIN_ID다', async () => {
        await service.register({ loginId: 'player01', password: 'pass1234' });

        expect((await errorOf(service.register({ loginId: 'player01', password: 'other999' }))).code).toBe('DUPLICATE_LOGIN_ID');
    });

    test('티켓을 발급하지 않는다', async () => {
        await service.register({ loginId: 'player01', password: 'pass1234' });

        expect(tickets.count()).toBe(0);
    });
});

describe('AuthService.login', () => {
    beforeEach(async () => {
        // bcrypt 비용을 낮춰 테스트 시간을 줄인다. 검증은 해시에 적힌 비용을 따르므로 판정은 같다.
        await accounts.create({ loginId: 'player01', passwordHash: await bcrypt.hash('pass1234', 4) });
    });

    test('성공하면 티켓을 돌려준다. 티켓은 UUID이고 그 계정의 번호를 가리킨다', async () => {
        const { ticket } = await service.login({ loginId: 'player01', password: 'pass1234' });

        expect(ticket).toMatch(UUID_PATTERN);
        expect(tickets.accountIdOf(ticket)).toBe(accounts.get('player01')!.accountId);
        expect(tickets.count()).toBe(1);
    });

    test('성공하면 마지막 로그인 시각을 갱신한다', async () => {
        await service.login({ loginId: 'player01', password: 'pass1234' });

        expect(accounts.get('player01')!.lastLoginAt).not.toBeNull();
    });

    test('다시 로그인하면 새 티켓을 발급하고 이전 티켓을 지우지 않는다', async () => {
        const first = await service.login({ loginId: 'player01', password: 'pass1234' });
        const second = await service.login({ loginId: 'player01', password: 'pass1234' });

        expect(second.ticket).not.toBe(first.ticket);
        expect(tickets.count()).toBe(2);
    });

    test('없는 로그인 ID면 INVALID_CREDENTIALS이고 티켓을 발급하지 않는다', async () => {
        const err = await errorOf(service.login({ loginId: 'nobody', password: 'pass1234' }));

        expect(err.code).toBe('INVALID_CREDENTIALS');
        expect(tickets.count()).toBe(0);
    });

    test('틀린 비밀번호면 INVALID_CREDENTIALS이고 티켓을 발급하지 않고 로그인 시각도 갱신하지 않는다', async () => {
        const err = await errorOf(service.login({ loginId: 'player01', password: 'wrong999' }));

        expect(err.code).toBe('INVALID_CREDENTIALS');
        expect(tickets.count()).toBe(0);
        expect(accounts.get('player01')!.lastLoginAt).toBeNull();
    });

    test('없는 로그인 ID와 틀린 비밀번호는 같은 응답이다', async () => {
        const unknownId = await errorOf(service.login({ loginId: 'nobody', password: 'pass1234' }));
        const wrongPassword = await errorOf(service.login({ loginId: 'player01', password: 'wrong999' }));

        expect({ code: unknownId.code, status: unknownId.status, message: unknownId.message, details: unknownId.details })
            .toEqual({ code: wrongPassword.code, status: wrongPassword.status, message: wrongPassword.message, details: wrongPassword.details });
    });

    test('정지 중이면 ACCOUNT_BANNED와 정지 기한을 주고, 티켓을 발급하지 않고 로그인 시각도 갱신하지 않는다', async () => {
        accounts.setBannedUntil('player01', new Date('2026-10-20T00:00:00.000Z'));

        const err = await errorOf(service.login({ loginId: 'player01', password: 'pass1234' }));

        expect(err.code).toBe('ACCOUNT_BANNED');
        expect(err.details).toEqual({ bannedUntil: '2026-10-20T00:00:00.000Z' });
        expect(tickets.count()).toBe(0);
        expect(accounts.get('player01')!.lastLoginAt).toBeNull();
    });

    test('정지 중이어도 비밀번호가 틀리면 INVALID_CREDENTIALS다', async () => {
        accounts.setBannedUntil('player01', new Date('2026-10-20T00:00:00.000Z'));

        const err = await errorOf(service.login({ loginId: 'player01', password: 'wrong999' }));

        expect(err.code).toBe('INVALID_CREDENTIALS');
    });

    test('정지 기한이 지난 계정은 로그인할 수 있다', async () => {
        accounts.setBannedUntil('player01', new Date('2026-10-19T11:59:59.999Z'));

        const { ticket } = await service.login({ loginId: 'player01', password: 'pass1234' });

        expect(tickets.accountIdOf(ticket)).toBe(accounts.get('player01')!.accountId);
    });

    test('정지 기한이 지금과 같으면 로그인할 수 있다', async () => {
        accounts.setBannedUntil('player01', NOW);

        await expect(service.login({ loginId: 'player01', password: 'pass1234' })).resolves.toHaveProperty('ticket');
    });
});
