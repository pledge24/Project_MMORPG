import dotenv from "dotenv";

dotenv.config();

const configs = {
    // AuthServer
    port: process.env.PORT,

    // UserDB
    UserDBConfig: {
        server: process.env.DB_DEV_SERVER,
        port: parseInt(process.env.DB_DEV_PORT),
        pool: {
            max: 5,
            min: 1,
            idleTimeoutMillis: 30000
        },
        options: {
            encrypt: false,
            datebase: process.env.DB_DEV_DATEBASE,
            trustServerCertificate: true,
        },
        authentication: {
            type: 'default',
            options: {
                userName: process.env.DB_DEV_USERNAME,
                password: process.env.DB_DEV_PASSOWRD,
            },
        },
    },

    // Redis
    redisHost: process.env.REDIS_HOST,
    redisPort: process.env.REDIS_PORT,
    redisPassword: process.env.REDIS_PASSWORD
};

export default configs;