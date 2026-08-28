# DualFire Codex Model Routing

DualFire의 프로젝트 로컬 라우팅 기준이다. 품질 85점 이상인 결과 중 벽시계 시간, 출력 토큰, 낮은 모델 등급 순으로 선택한다. 기준선은 Codex 0.145.0, clean HEAD `d7f9e9a`, 2026-08-28에 측정했다.

## 검증된 모델

`codex debug models`에서 다음 조합을 재확인했다.

| 모델 | 프로젝트 용도 | effort |
| --- | --- | --- |
| `gpt-5.4-mini` | 파일·심벌·호출 경로 검색 | low |
| `gpt-5.6-luna` | 목록화, 로그·DataTable 구조화 추출 | low, medium |
| `gpt-5.3-codex-spark` | 사용자 감독하의 검증된 국소 수정 | medium |
| `gpt-5.6-terra` | 일반 구현, 복합 분석, 독립 리뷰 | medium, high |
| `gpt-5.6-sol` | 명세, 분해, 통합, 고위험 판정 | medium, high, xhigh |

## 12회 벤치마크

분석 점수는 정확성 50, 근거 20, 범위 준수 20, 형식 준수 10이다. 구현은 정답 동작, 수정 범위, 빌드, 관련 Automation Test를 평가했다. 토큰은 입력/출력이며 캐시 입력을 포함한다.

| 과제 | 후보 | 점수 | 시간 | 토큰 | 결과 |
| --- | --- | ---: | ---: | ---: | --- |
| 파일·심벌·호출 경로 | Mini Low | 90 | 58.2s | 115,599 / 3,460 | 선택 |
| 파일·심벌·호출 경로 | Luna Low | 96 | 92.9s | 142,985 / 2,973 | 통과 |
| DataTable/C++ 구조화 추출 | Luna Low | 98 | 30.6s | 44,089 / 1,480 | 선택 |
| DataTable/C++ 구조화 추출 | Luna Medium | 97 | 34.3s | 44,172 / 1,694 | 통과 |
| Stage 생명주기·C++/Blueprint 경계 | Terra Medium | 97 | 45.4s | 173,462 / 2,643 | 선택 |
| Stage 생명주기·C++/Blueprint 경계 | Terra High | 98 | 67.5s | 170,085 / 3,024 | 통과 |
| 다중 파일 회귀 리뷰 | Terra High | 97 | 60.0s* | 414,619 / 3,038 | 선택 |
| 다중 파일 회귀 리뷰 | Sol High | 98 | 161.8s | 458,474 / 6,326 | 통과 |
| 큰 DeltaTime Shield 회복 수정 | Spark Medium | 95 | 11.9s | 52,630 / 3,196 | 선택 |
| 큰 DeltaTime Shield 회복 수정 | Terra Medium | 100 | 30.0s | 102,033 / 917 | 통과 |
| 9개 Spawn Anchor 중복 수정 | Spark Medium | 100 | 23.0s | 195,684 / 3,730 | 선택 |
| 9개 Spawn Anchor 중복 수정 | Terra Medium | 100 | 44.0s | 184,559 / 1,573 | 통과 |

`* Terra High 리뷰 시간은 실행 로그 경계 기준 0.1초 정밀도가 보존되지 않아 초 단위로 반올림했다.

두 구현 조합 모두 `DualFireEditor Win64 Development` 빌드와 관련 테스트를 통과했다.

- `DualFire.Health.ShieldRecoveryLargeDelta`: Spark Medium, Terra Medium 모두 Success
- `DualFire.Stage.SpawnAnchors`: Spark Medium, Terra Medium 모두 Success

벤치마크의 회귀·테스트는 임시 Worktree 전용이며 제품 코드에는 반영하지 않았다.

## Unreal 전용 라우팅

| 작업 | 기본 배정 | 승격 조건 |
| --- | --- | --- |
| 파일, 심벌, 호출부 검색 | Mini Low | 관계 판단이 필요하면 Terra Medium |
| 목록화, 로그·데이터 추출 | Luna Low | 반복 구조화는 Luna Medium |
| 국소 C++ 회귀 수정 | Spark Medium | 다중 파일, 설계, 긴 검증이면 Terra Medium |
| 일반 C++, Blueprint, DataTable 구현 | Terra Medium | 복합 디버깅·독립 리뷰는 Terra High |
| 다중 파일 회귀·검증 누락 리뷰 | Terra High | 결과 상충·고위험 판정은 Sol High |
| 명세, TASK-ID 분해, 통합 | Sol Medium | 아키텍처·고위험은 Sol High/xhigh |

## 운영 규칙

1. Sol만 `Goal.md`와 worksheet 상태를 수정한다.
2. 작업자는 배정된 TASK-ID와 write scope만 다루며 하위 에이전트를 생성하지 않는다.
3. 파일별 writer는 한 명이다. 독립 장기 수정은 별도 Git Worktree를 쓴다.
4. 읽기 작업만 병렬화하고 쓰기는 파일 소유권을 분리한다.
5. Unreal Editor/MCP는 한 시점에 한 coordinator만 소유한다.
6. Blueprint compile, asset save, PIE는 병렬 실행하지 않는다.
7. 빌드, Blueprint compile, Automation Test, PIE는 서로 다른 증거다. 실행하지 않은 검증은 `not_run`으로 남긴다.
8. Mini와 Luna는 구현·설계·최종 판정을 하지 않는다.
9. Spark는 사용자 감독하의 짧고 검증 가능한 수정에만 쓴다.
10. Terra Medium이 설계나 범위 판단에서 막히면 Terra effort를 올리지 말고 Sol High로 반환한다.
11. Sol High/xhigh는 아키텍처, 고위험 판정, 상충 결과 감사에만 쓴다.
12. 품질 하한을 지키는 가장 낮고 빠른 모델을 우선한다.

## 반환 형식

모든 작업자는 `STATUS`, `TASK-ID`, 수행 작업, 변경 파일, 검증 결과, 완료조건별 PASS/FAIL, 남은 위험, 모델/effort, 다음 Sol 행동을 반환한다.
