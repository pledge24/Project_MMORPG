import bcrypt from 'bcrypt';
import { beforeEach, describe, expect, test } from 'vitest';

import { MemoryAccountRepository } from '../accounts/memoryAccountRepository.js';
import { AppError } from '../errors.js';
import { RegisterService } from './registerService.js';

let accounts: MemoryAccountRepository;
let service: RegisterService;

beforeEach(() => {
    accounts = new MemoryAccountRepository();
    service = new RegisterService(accounts);
});

async function codeOf(promise: Promise<unknown>): Promise<string> {
    const err = await promise.then(() => undefined, (e: unknown) => e);
    expect(err).toBeInstanceOf(AppError);
    return (err as AppError).code;
}

describe('RegisterService.register', () => {
    test('계정을 만들고 소문자로 바꾼 로그인 ID를 돌려준다', async () => {
        const result = await service.register({ loginId: 'Player01', password: 'pass1234' });

        expect(result).toEqual({ loginId: 'player01' });
        const stored = accounts.findByLoginId('player01');
        expect(stored).toBeDefined();
        expect(await bcrypt.compare('pass1234', stored!.passwordHash)).toBe(true);
    });

    test('같은 로그인 ID로 다시 가입하면 DUPLICATE_LOGIN_ID다', async () => {
        await service.register({ loginId: 'player01', password: 'pass1234' });

        expect(await codeOf(service.register({ loginId: 'player01', password: 'other999' }))).toBe('DUPLICATE_LOGIN_ID');
    });

    test('대소문자만 다른 로그인 ID도 같은 로그인 ID다', async () => {
        await service.register({ loginId: 'player01', password: 'pass1234' });

        expect(await codeOf(service.register({ loginId: 'PLAYER01', password: 'pass1234' }))).toBe('DUPLICATE_LOGIN_ID');
    });

    test.each([
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
    ])('%s이면 VALIDATION_FAILED이고 계정을 만들지 않는다', async (_case, input) => {
        expect(await codeOf(service.register(input))).toBe('VALIDATION_FAILED');
        expect(accounts.count()).toBe(0);
    });
});
