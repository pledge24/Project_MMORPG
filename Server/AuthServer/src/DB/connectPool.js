import sql from 'mssql';
import configs from '../Config/configs.js';

const connectionPool = sql.connectionPool(configs.UserDBConfig)
    .connect()
    .then((pool) => {
        console.log('UserDB 연결 성공');
        return pool;
    })
    .catch((err) => {
        console.log('err ', err);
    });

export default connectionPool;