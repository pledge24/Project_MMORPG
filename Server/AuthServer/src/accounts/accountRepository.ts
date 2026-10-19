/** 계정 번호. AccountDB의 BIGINT를 10진수 문자열로 다룬다. 계정 번호로 계산하지 않으므로 숫자로 바꾸지 않는다. */
export type AccountId = string;

export interface NewAccount {
    /** 소문자로 바꾸고 형식 검사를 통과한 값 */
    loginId: string;
    passwordHash: string;
}

export type CreateAccountResult =
    | { kind: 'created'; accountId: AccountId }
    | { kind: 'duplicate' };

/** 로그인 판정에 쓰는 계정 정보 */
export interface LoginAccount {
    accountId: AccountId;
    passwordHash: string;
    /** null이면 정상이다. 값이 있으면 그 시각까지 로그인을 거절한다. */
    bannedUntil: Date | null;
}

/**
 * 계정을 저장하는 곳.
 *
 * 회원가입과 로그인의 판정은 이 인터페이스만 안다. DB를 아는 코드는 구현뿐이다.
 */
export interface AccountRepository {
    /** 같은 로그인 ID의 계정이 이미 있으면 만들지 않고 duplicate를 돌려준다. */
    create(account: NewAccount): Promise<CreateAccountResult>;

    /** 소문자로 바꾼 로그인 ID로 찾는다. 없으면 undefined다. */
    findByLoginId(loginId: string): Promise<LoginAccount | undefined>;

    /** 마지막 로그인 시각을 지금으로 바꾼다. */
    recordLogin(accountId: AccountId): Promise<void>;
}
