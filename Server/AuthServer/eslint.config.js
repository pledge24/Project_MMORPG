// ESLint 10의 flat config다. 이 파일이 없으면 eslint가 설정을 찾지 못해 실행 자체가 실패한다.
import js from '@eslint/js';
import globals from 'globals';
import tseslint from 'typescript-eslint';

export default [
    // 의존성은 검사하지 않는다.
    { ignores: ['node_modules/**'] },

    js.configs.recommended,

    {
        files: ['**/*.js', '**/*.ts'],
        languageOptions: {
            ecmaVersion: 'latest',
            // package.json의 "type": "module"과 맞춘다.
            sourceType: 'module',
            globals: {
                ...globals.node,
            },
        },
    },

    // TypeScript 파일에만 적용한다.
    // TODO: tsconfig가 생기면 타입 정보를 쓰는 규칙을 켠다
    ...tseslint.configs.recommended.map((config) => ({ ...config, files: ['**/*.ts'] })),
    {
        files: ['**/*.ts'],
        rules: {
            // Express는 인자 개수로 에러 핸들러를 알아보므로, 쓰지 않는 인자도 지울 수 없다. 그런 인자는 _로 시작한다.
            '@typescript-eslint/no-unused-vars': ['error', { argsIgnorePattern: '^_' }],
        },
    },
];
