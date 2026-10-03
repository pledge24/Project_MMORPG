# Triage Labels

The skills speak in terms of five canonical triage roles. This file maps those roles to the actual label strings used in this repo's issue tracker.

| Label in mattpocock/skills | Label in our tracker | Meaning                                  |
| -------------------------- | -------------------- | ---------------------------------------- |
| `needs-triage`             | `needs-triage`       | Maintainer needs to evaluate this issue  |
| `needs-info`               | `needs-info`         | Waiting on reporter for more information |
| `ready-for-agent`          | `ready-for-agent`    | Fully specified, ready for an AFK agent  |
| `ready-for-human`          | `ready-for-human`    | Requires human implementation            |
| `wontfix`                  | `wontfix`            | Will not be actioned                     |

When a skill mentions a role (e.g. "apply the AFK-ready triage label"), use the corresponding label string from this table.

Edit the right-hand column to match whatever vocabulary you actually use.

All five labels already exist in this repo's GitHub Issues (with Korean descriptions), so `/triage` applies them rather than creating them.

## Labels outside the triage roles

| Label | 뜻 | 붙이는 조건 |
| --- | --- | --- |
| `tech-debt` | `docs/tech-debt.md`에 항목이 있는 결함을 고치는 이슈다 | tech-debt 항목에 대응하는 이슈를 열 때 트리아지 라벨과 함께 붙인다. 이미 열린 이슈의 결함을 나중에 tech-debt에 적었으면 그때 붙인다 |
