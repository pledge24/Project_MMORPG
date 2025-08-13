import dotenv from "dotenv";

dotenv.config();

const configs = {

    // AuthServer
    port: process.env.PORT,

    // UserDB
    UserDBConfig: {
        connectionString: process.env.DB_CONNECTION_STRING,
        driver: process.env.DB_DRIVER,

        // DB 커넥션 풀 설정
        pool: {                                         
            max: parseInt(process.env.DB_MAX_CONNECTION),
            min: parseInt(process.env.DB_MIN_CONNECTION),
            idleTimeoutMillis: parseInt(process.env.DB_IDLE_TIMEOUT)
        },

        // DB 추가 옵션
        options: {                                      
            encrypt: false,
            trustServerCertificate: true,
            enableArithAbort: true,
            integratedSecurity: true
        }
    },

    // Redis
    redisHost: process.env.REDIS_HOST,
    redisPort: process.env.REDIS_PORT,
    accessTokenTTL: process.env.ACCESS_TOKEN_TTL
};

export default configs;
