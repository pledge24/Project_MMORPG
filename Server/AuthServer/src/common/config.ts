// 기동할 때 환경 변수를 한 번 읽어 검사한다. 틀린 값으로 포트를 열면 첫 요청에서야 DB나 Redis 접속이 터진다.

/** 기동에 필요한 환경 변수. 예시 파일(.env.example)의 키와 같아야 한다. */
export const REQUIRED_ENV_KEYS = [
    'PORT',
    'DB_CONNECTION_STRING',
    'DB_MAX_CONNECTION',
    'DB_MIN_CONNECTION',
    'DB_IDLE_TIMEOUT',
    'REDIS_HOST',
    'REDIS_PORT',
    'TICKET_TTL_SECONDS',
] as const;

type EnvKey = (typeof REQUIRED_ENV_KEYS)[number];

export interface Config {
    port: number;
    accountDb: {
        connectionString: string;
        pool: { max: number; min: number; idleTimeoutMillis: number };
    };
    redis: { host: string; port: number };
    /** 인증 티켓의 유효 시간(초). 월드 선택 응답부터 게임 서버의 로비 접속까지의 시간이다. */
    ticketTtlSeconds: number;
}

/** 설정 검사에서 찾은 문제를 모두 담는다. */
export class ConfigError extends Error {
    readonly problems: string[];

    constructor(problems: string[]) {
        super(`설정이 올바르지 않다:\n- ${problems.join('\n- ')}`);
        this.name = 'ConfigError';
        this.problems = problems;
    }
}

/** 빠지거나 형식이 틀린 키가 하나라도 있으면 그 목록을 담은 ConfigError를 던진다. */
export function loadConfig(env: Record<string, string | undefined>): Config {
    const problems: string[] = [];

    const readText = (key: EnvKey): string => {
        const value = env[key];
        if (value === undefined || value === '') {
            problems.push(`${key}가 없다`);
            return '';
        }
        return value;
    };

    const readInteger = (key: EnvKey, min: number, max = Number.MAX_SAFE_INTEGER): number => {
        const value = readText(key);
        if (value === '') {
            return NaN;
        }
        const parsed = /^\d+$/.test(value) ? Number(value) : NaN;
        if (!(parsed >= min && parsed <= max)) {
            problems.push(`${key}는 ${min} 이상 ${max} 이하의 정수여야 한다: '${value}'`);
        }
        return parsed;
    };

    const config: Config = {
        port: readInteger('PORT', 1, 65535),
        accountDb: {
            connectionString: readText('DB_CONNECTION_STRING'),
            pool: {
                max: readInteger('DB_MAX_CONNECTION', 1),
                min: readInteger('DB_MIN_CONNECTION', 0),
                idleTimeoutMillis: readInteger('DB_IDLE_TIMEOUT', 0),
            },
        },
        redis: {
            host: readText('REDIS_HOST'),
            port: readInteger('REDIS_PORT', 1, 65535),
        },
        ticketTtlSeconds: readInteger('TICKET_TTL_SECONDS', 1),
    };

    const { min, max } = config.accountDb.pool;
    if (min > max) {
        problems.push(`DB_MIN_CONNECTION(${min})이 DB_MAX_CONNECTION(${max})보다 크다`);
    }

    if (problems.length > 0) {
        throw new ConfigError(problems);
    }
    return config;
}
