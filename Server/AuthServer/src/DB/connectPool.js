import sql from 'mssql/msnodesqlv8.js';
import configs from '../Config/configs.js';

const connectionPool = new sql.ConnectionPool(configs.UserDBConfig);
    
export default connectionPool;
