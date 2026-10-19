import bcrypt from 'bcrypt';
import { randomUUID } from 'node:crypto';

/** 비밀번호를 해시할 때의 bcrypt 비용 */
const BCRYPT_COST = 10;

// 대조할 해시가 없을 때 대신 비교하는 해시. 모듈을 로드할 때 한 번 만든다.
const PASSWORD_HASH_FOR_MISSING_ACCOUNT = bcrypt.hashSync(randomUUID(), BCRYPT_COST);

export function hashPassword(password: string): Promise<string> {
    return bcrypt.hash(password, BCRYPT_COST);
}

/**
 * 비밀번호가 해시와 맞는지 본다. 해시가 없으면(없는 계정) 더미 해시로 비교를 한 번 하고 false를 돌려준다.
 * 비교를 건너뛰면 응답 시간의 차이로 계정이 있는지를 밖에서 알아낼 수 있다.
 */
export async function verifyPassword(password: string, passwordHash: string | undefined): Promise<boolean> {
    const matches = await bcrypt.compare(password, passwordHash ?? PASSWORD_HASH_FOR_MISSING_ACCOUNT);
    return passwordHash !== undefined && matches;
}
