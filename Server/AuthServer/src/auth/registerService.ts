import bcrypt from 'bcrypt';

import type { AccountRepository } from '../accounts/accountRepository.js';
import { AppError } from '../errors.js';
import { parseCredentials } from './credentials.js';

export const BCRYPT_COST = 10;

/**
 * 회원가입 판정.
 *
 * 로그인 ID와 비밀번호를 검사하고 계정을 만든다. 인증 티켓은 발급하지 않는다.
 */
export class RegisterService {
    private readonly accounts: AccountRepository;

    constructor(accounts: AccountRepository) {
        this.accounts = accounts;
    }

    /** 형식이 틀리면 VALIDATION_FAILED, 같은 로그인 ID가 있으면 DUPLICATE_LOGIN_ID를 던진다. */
    async register(body: unknown): Promise<{ loginId: string }> {
        const { loginId, password } = parseCredentials(body);

        // 중복 확인을 따로 하지 않고 저장소의 유일성 검사에 맡긴다. 확인과 생성 사이에 같은 로그인 ID가 끼어들 수 없다.
        const passwordHash = await bcrypt.hash(password, BCRYPT_COST);
        const result = await this.accounts.create({ loginId, passwordHash });
        if (result.kind === 'duplicate') {
            throw new AppError('DUPLICATE_LOGIN_ID');
        }

        return { loginId };
    }
}
