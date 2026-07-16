# DualFire Codex Model Routing

이 파일은 DualFire 프로젝트 로컬 Codex 운영체계의 모델 배정 기준이다. `Goal.md`와 worksheet의 목표 내용은 변경하지 않고, Sol이 지휘하고 Terra, Luna, Mini, Spark를 역할과 추론 단계에 따라 배정한다.

## Model ID 검증

검증 기준: `C:\Users\kang9\.codex\models_cache.json`

검증 시각: `2026-07-15T11:42:33.404295300Z`

| 모델 ID | 표시 이름 | 지원 추론 단계 |
| --- | --- | --- |
| `gpt-5.6-sol` | GPT-5.6-Sol | low, medium, high, xhigh, max, ultra |
| `gpt-5.6-terra` | GPT-5.6-Terra | low, medium, high, xhigh, max, ultra |
| `gpt-5.6-luna` | GPT-5.6-Luna | low, medium, high, xhigh, max |
| `gpt-5.5` | GPT-5.5 | low, medium, high, xhigh |
| `gpt-5.4` | GPT-5.4 | low, medium, high, xhigh |
| `gpt-5.4-mini` | GPT-5.4-Mini | low, medium, high, xhigh |
| `gpt-5.3-codex-spark` | GPT-5.3-Codex-Spark | low, medium, high, xhigh |

## 추론 단계 대응

| 앱 단계 | config 값 |
| --- | --- |
| Light | `low` |
| 중간 | `medium` |
| 높음 | `high` |
| 매우 높음 | `xhigh` |
| Max | `max` |
| Ultra | `ultra` |

## 모델 x 추론 단계 라우팅 매트릭스

| 모델 | Light | 중간 | 높음 | 매우 높음 |
| --- | --- | --- | --- | --- |
| 5.6 Sol | 빠른 최종 판정. 대부분 과투자 | 기본 지휘자: 분해, 배정, 통합 | 아키텍처, 난해한 문제, 중요 리뷰 | 고위험 결정, 상충 결과 판정, 최종 감사 |
| 5.6 Terra | 코드 탐색, 제한된 수정 | 기본 작업자: 구현, 조사, 도구 사용 | 복합 구현, 다중 파일 디버깅, 독립 리뷰 | 보통 Sol High로 올리는 편이 효율적 |
| 5.6 Luna | 추출, 분류, 스캔, 포맷 변환 | 반복 변환, 구조화, 정형 요약 | 예외가 있는 명확한 배치 작업 | 사용하지 말고 Terra 또는 Sol로 승격 |
| 5.5 | 결과 비교 | 기존 5.5 워크플로 유지 | 회귀 비교 | 신규 기본 경로에는 비추천 |
| 5.4 | 작은 기존 작업 | 5.4 고정 프로젝트 | 호환성, 재현 목적 | 신규 작업에는 비추천 |
| 5.4 Mini | grep, 파일 맵, 목록화 | 기계적 수정, 테스트 생성, 문서 정리 | 제한된 작업만 | 실패하면 Terra로 승격 |
| 5.3 Codex Spark | 즉석 질의, 코드 탐색 | 실시간 국소 수정, 빠른 반복 | 복잡해지면 Terra로 전환 | 부적합 |

## 강제 라우팅 규칙

1. 작업이 모호하면 먼저 Sol Medium 또는 High가 명세화한다.
2. 명세와 완료조건이 확정되면 Terra 또는 Luna로 하향 배정한다.
3. 일반 구현의 기본값은 Terra Medium이다.
4. 다중 파일 수정, 복잡한 디버깅, 독립 리뷰는 Terra High다.
5. 추출, 분류, 변환, 목록화처럼 명확한 작업은 Luna Light다.
6. 여러 단계가 있지만 결과 형식이 명확한 반복 작업은 Luna Medium이다.
7. Luna High는 예외 처리가 있지만 범위와 완료조건이 명확할 때만 사용한다.
8. Luna xhigh는 사용하지 않는다.
9. Luna가 요구사항 해석이나 파일 관계를 반복해서 놓치면 Terra Medium으로 승격한다.
10. Terra Medium이 설계 판단에서 실패하면 Terra xhigh가 아니라 Sol High로 승격한다.
11. Sol Medium은 상시 지휘, 통합의 기본값이다.
12. Sol High는 설계, 난해한 문제, 중요 리뷰에 사용한다.
13. Sol xhigh는 고위험 결정, 상충된 검토 결과, 최종 감사에만 사용한다.
14. Sol을 단순 구현, 대량 추출, 원시 로그 분석에 사용하지 않는다.
15. 5.4 Mini Light는 파일 검색, 목록화, 코드 위치 확인에 사용한다.
16. 5.4 Mini Medium은 기계적 수정, 테스트 후보 생성, 문서 정리에 사용한다.
17. 5.4 Mini가 범위 밖 판단을 요구받으면 Terra로 승격한다.
18. Spark Light는 즉석 코드 질의와 탐색에 사용한다.
19. Spark Medium은 사용자가 직접 지켜보는 짧은 수정 반복에 사용한다.
20. Spark 작업이 여러 파일, 설계, 장시간 검증으로 확대되면 Terra로 전환한다.
21. 5.5와 5.4는 신규 작업의 기본 모델로 사용하지 않는다.
22. 5.5와 5.4는 기존 워크플로 재현, 회귀 비교, 호환성 확인에만 사용한다.
23. 약한 모델에 xhigh를 주어 문제를 계속 해결하게 하지 않는다.
24. 완료조건을 내는 가장 낮은 모델과 추론 단계를 우선 사용한다.

## 승격과 하향

- 모호한 요구사항은 Sol Medium이 분해하고, 위험하거나 상충되는 요구사항은 Sol High로 올린다.
- 구현 가능한 작업은 Terra Medium으로 내린다.
- 단순 추출, 분류, 변환, 목록화는 Luna Light 또는 Mini Light로 내린다.
- 반복 구조화와 정형 요약은 Luna Medium으로 배정한다.
- 다중 파일 구현, 복잡한 디버깅, 독립 리뷰는 Terra High로 올린다.
- 설계 판단 실패, 범위 판단 실패, 요구사항 해석 실패는 Sol High로 반환한다.
- 같은 유형의 실패가 두 번 반복되면 한 단계 올리고, 세 번째 실패는 `BLOCKED`로 반환한다.

## 하위 에이전트와 별도 세션

하위 에이전트를 쓰는 경우:

- 같은 최종 결과를 위한 탐색, 조사, 테스트
- 짧은 독립 작업
- Sol이 결과를 수집해 판단해야 하는 작업
- 읽기 중심 작업
- 원시 로그 분리 요약

별도 세션을 쓰는 경우:

- 독립 산출물
- 장시간 구현
- 사용자가 각 트랙을 별도로 조정해야 하는 작업
- 별도 branch와 diff 검토가 필요한 작업
- 서로 다른 목표, 권한, 컨텍스트가 필요한 작업

파일을 수정하는 별도 세션은 Git Worktree를 사용한다. 새 세션만 만들고 같은 checkout을 동시에 수정하게 하지 않는다.

## 작업 위험도별 검토 모델

| 위험도 | 예시 | 검토 모델 |
| --- | --- | --- |
| 낮음 | 파일 위치 확인, 목록화, 포맷 정리 | Mini Light, Luna Light |
| 중간 | 단일 기능 구현, 문서 정리, 테스트 후보 생성 | Terra Medium, Luna Medium |
| 높음 | 다중 파일 변경, 복잡한 로직, 테스트 누락 가능성 | Terra High reviewer |
| 매우 높음 | 아키텍처 변경, 상충된 리뷰, merge 최종 판정 | Sol High 또는 Sol xhigh |

## 모델 선택 예시

| 작업 | 기본 배정 |
| --- | --- |
| `Source/`에서 특정 클래스 사용처 찾기 | 5.4 Mini Light |
| 여러 문서에서 시스템 목록 추출 | Luna Light |
| 추출 결과를 표준 worksheet 형식으로 재구성 | Luna Medium |
| 일반 C++ 구현 | Terra Medium |
| 여러 C++ 파일과 Blueprint 경계가 얽힌 디버깅 | Terra High |
| 코드 리뷰 결과가 상충됨 | Sol High |
| 최종 DONE 판정과 인계 | Sol Medium, 필요 시 Sol High |

## 사용 금지 조합

- Luna xhigh는 사용하지 않는다.
- Luna에 아키텍처, 최종 판단, 요구사항 해석을 맡기지 않는다.
- Mini에 구현 방향, 설계 판단, 범위 판단을 맡기지 않는다.
- Spark에 장시간 검증, 다중 파일 설계, 독립 산출물을 맡기지 않는다.
- 5.5와 5.4를 신규 작업의 기본 모델로 쓰지 않는다.
- 약한 모델의 xhigh로 반복 실패를 밀어붙이지 않는다.
- Sol을 단순 구현, 대량 추출, 원시 로그 분석에 쓰지 않는다.

## Max와 Ultra

- Max는 하나의 난해한 작업을 단일 모델이 더 깊게 판단해야 할 때만 사용한다.
- Ultra는 의미 있게 분리 가능한 복합 작업에서 자동 하위 에이전트 위임이 필요할 때만 사용한다.
- 일반 작업에는 Max나 Ultra를 사용하지 않는다.
- Sol Ultra는 다중 하위 에이전트 위임이 실제로 필요한 복합 작업에만 사용한다.
- Terra Ultra는 기본 경로가 아니며, Terra High로 부족하고 Sol 지휘보다 단일 작업 지속이 더 적합할 때만 검토한다.

## 작업 소유권

- `Goal.md`: 사용자와 Sol만 수정한다.
- worksheet: Sol만 상태를 수정한다.
- 작업 패킷: Sol이 작성한다.
- 구현 파일: 배정된 작업자 한 명만 수정한다.
- 리뷰 결과: reviewer가 작성한다.
- 통합, merge, 최종 DONE 판정: Sol만 수행한다.

## 결과 반환 형식

모든 작업자는 다음 형식으로 반환한다.

1. `STATUS: DONE | BLOCKED | NEEDS_DECISION`
2. `TASK-ID`
3. 수행한 작업
4. 변경 파일
5. 실행한 검증과 결과
6. 완료조건별 `PASS/FAIL`
7. 남은 위험과 가정
8. 사용한 모델과 추론 단계
9. Sol이 다음에 수행할 정확한 행동
