import bcrypt from 'bcrypt';
import { beforeEach, describe, expect, test } from 'vitest';

import { MemoryAccountRepository } from '../accounts/memoryAccountRepository.js';
import { AppError } from '../errors.js';
import { MemoryTicketRepository } from '../tickets/memoryTicketRepository.js';
import { LoginService } from './loginService.js';

const NOW = new Date('2026-10-19T12:00:00.000Z');
const UUID_PATTERN = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;

let accounts: MemoryAccountRepository;
let tickets: MemoryTicketRepository;
let service: LoginService;

beforeEach(async () => {
    accounts = new MemoryAccountRepository();
    tickets = new MemoryTicketRepository();
    service = new LoginService(accounts, tickets, () => NOW);

    // bcrypt 비용을 낮춰 테스트 시간을 줄인다. 검증은 해시에 적힌 비용을 따르므로 판정은 같다.
    await accounts.create({ loginId: 'player01', passwordHash: await bcrypt.hash('pass1234', 4) });
});

async function errorOf(promise: Promise<unknown>): Promise<AppError> {
    const err = await promise.then(() => undefined, (e: unknown) => e);
    expect(err).toBeInstanceOf(AppError);
    return err as AppError;
}

describe('LoginService.login', () => {
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

    test('로그인 ID는 소문자로 바꿔서 찾는다', async () => {
        const { ticket } = await service.login({ loginId: 'PLAYER01', password: 'pass1234' });

        expect(tickets.accountIdOf(ticket)).toBe(accounts.get('player01')!.accountId);
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

    test.each([
        ['본문이 객체가 아님', 'player01'],
        ['로그인 ID 없음', { password: 'pass1234' }],
        ['로그인 ID가 4자 미만', { loginId: 'abc', password: 'pass1234' }],
        ['비밀번호 없음', { loginId: 'player01' }],
        ['비밀번호에 특수문자', { loginId: 'player01', password: 'pass!234' }],
    ])('%s이면 VALIDATION_FAILED이고 티켓을 발급하지 않는다', async (_case, input) => {
        expect((await errorOf(service.login(input))).code).toBe('VALIDATION_FAILED');
        expect(tickets.count()).toBe(0);
    });
});
