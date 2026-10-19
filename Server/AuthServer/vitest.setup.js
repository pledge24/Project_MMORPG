// 테스트 프로세스에 .env를 읽어 둔다. npm start는 Node의 --env-file 옵션으로 읽지만 vitest는 그 옵션을 받지 않는다.
// 옛 로그인 경로의 설정 모듈이 process.env를 읽기 때문이다. 새 설정 모듈의 테스트는 .env를 쓰지 않는다. 파일이 없으면 넘어간다.
try {
    process.loadEnvFile('.env');
}
catch (err) {
    if (err.code !== 'ENOENT') {
        throw err;
    }
}
