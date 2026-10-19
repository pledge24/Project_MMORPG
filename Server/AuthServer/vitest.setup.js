// 테스트 프로세스에 .env를 읽어 둔다. npm start는 Node의 --env-file 옵션으로 읽지만 vitest는 그 옵션을 받지 않는다.
// 파일이 없으면 넘어간다. 그때는 설정 테스트가 빠진 키를 알린다.
try {
    process.loadEnvFile('.env');
}
catch (err) {
    if (err.code !== 'ENOENT') {
        throw err;
    }
}
