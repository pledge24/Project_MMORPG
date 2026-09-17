---
name: session-report
description: Generate a human-readable report of the work actually performed during the current Codex session. Use when a long coding session is nearing completion and a developer wants to understand what changed, why it changed, what was discovered, how it was validated, and what should be reviewed before merging the work.
disable-model-invocation: true
---

# Session Report

Generate a human-readable report describing what actually happened during the current coding session.

The report is intended for a developer who may not have followed the session and needs to understand the work before reviewing or merging the resulting changes.

The primary goal is **understanding and reviewability**, not merely summarizing the git diff.

## Core Principles

### Report what actually happened

Reconstruct the work from evidence available in the current session and repository.

Prefer:

- actual code changes
- commands that were actually run
- tests that were actually executed
- errors that actually occurred
- discoveries made while investigating the codebase
- decisions made during implementation
- the final state of the repository

Do not describe planned work as completed work.

If the original plan and the final implementation differ, describe the difference.

### Do not invent information

Never claim that something happened unless there is evidence for it.

If something cannot be established from the available evidence, explicitly say:

- `Not verified`
- `Unknown`
- `Not observed during this session`

Do not infer that tests passed merely because the code appears correct.

Do not claim that a behavior is preserved unless it was established through code inspection or validation.

### Explain context, not just changes

A file list or diff summary is insufficient.

For every meaningful change, explain:

1. What changed?
2. Why was it changed?
3. How did the change affect the system?
4. What should a reviewer understand about this change?

Prefer explanations that allow a developer to understand the change without reading every changed line.

### Write for a developer who did not attend the session

Assume the reader:

- knows basic programming concepts
- may be unfamiliar with this repository
- did not see the investigation or implementation process
- will inspect the PR before merging it

Explain repository-specific concepts when they are necessary to understand the change.

Avoid unexplained internal terminology.

Do not over-explain ordinary programming concepts.

### Preserve important uncertainty

If the session encountered uncertainty, ambiguity, an unexpected dependency, an unverified assumption, or an unresolved issue, preserve it in the report.

Do not make the final implementation appear more certain or straightforward than the actual session was.

---

# Investigation

Before writing the report, reconstruct the session as accurately as possible.

Use the available conversation/session context first.

Then inspect the repository and git state as necessary to verify the final result.

At minimum, when applicable, inspect:

- current git status
- changed files
- relevant git diff
- recent commits created during the session
- tests or validation commands that were actually run
- relevant implementation files
- relevant configuration or dependency changes

Use additional repository investigation when it is necessary to explain the behavior or verify a claim.

Do not perform unrelated cleanup or modify application code while generating the report.

The purpose of this skill is to observe and explain the work, not to continue implementing the feature.

## Distinguish session work from pre-existing work

Be careful when the working tree already contained changes before the session.

If the available evidence allows you to distinguish them, separate:

- changes made during this session
- pre-existing changes
- changes whose origin cannot be established

Do not attribute unrelated pre-existing changes to the current session.

If the boundary cannot be determined confidently, say so.

---

# Reconstruct the Work

Build a concise mental timeline of the session.

Look for:

1. The problem that initiated the work
2. The intended outcome
3. Investigation performed
4. Important discoveries
5. Approaches attempted
6. Approaches abandoned or changed
7. Important implementation decisions
8. Actual code/configuration changes
9. Validation performed
10. Remaining risks or unresolved questions

Do not include every command or every intermediate thought.

Include intermediate work when it explains an important decision, unexpected behavior, bug, or change in direction.

A useful test is:

> "Would knowing this help a reviewer understand why the final code looks the way it does?"

If yes, include it.

If not, omit it.

---

# Required Report Structure

Generate the report using the following structure.

## 1. TL;DR

Summarize the entire session in approximately 3–5 sentences.

Answer:

- What problem was addressed?
- What was changed?
- What is the resulting behavior?
- Is there anything important the reviewer should know?

This section should allow a busy reviewer to understand the session without reading the rest of the document.

---

## 2. What Changed

Describe the meaningful changes produced by the session.

Group related changes by feature, behavior, or architectural area rather than mechanically listing files.

For each significant change, explain:

- what changed
- why it changed
- what behavior changed as a result

Include file paths when they help the reader locate the implementation.

Do not turn this section into a raw diff.

---

## 3. How It Works Now

Explain the resulting behavior from a system or user perspective.

When useful, describe:

```text
Before
  ↓
Old behavior

After
  ↓
New behavior
```

Use concrete examples when they make the change easier to understand.

This section should answer:

> "If I did not read the code, what would I now expect the system to do?"

If the change is internal and has no observable user-facing behavior, explain the relevant internal behavior instead.

---

## 4. Important Decisions

Document significant technical or architectural decisions made during the session.

For each decision, describe:

### Decision

What was chosen.

### Reason

Why it was chosen.

### Alternatives considered

Only include alternatives that were actually considered or are clearly evidenced by the session.

### Consequences

What this decision means for the resulting implementation.

Do not fabricate alternatives merely to make the report look complete.

Minor implementation choices do not belong here.

---

## 5. Discoveries

Record important things discovered while working in the repository.

Examples include:

- existing behavior that was initially unexpected
- hidden dependencies
- implicit assumptions
- relevant legacy behavior
- surprising control flow
- existing bugs
- limitations of the current architecture
- interactions between components
- constraints that affected implementation

Focus on discoveries that provide useful context for understanding the final change.

This section is particularly important when the final implementation differs from the obvious or initially expected approach.

---

## 6. Validation

Document what was actually verified.

Separate validation into:

### Executed

Commands, tests, builds, linting, type checks, manual checks, or other validation that actually occurred.

For each meaningful validation, provide the result.

Example:

```text
- `npm test -- foo.test.ts`
  Result: passed
```

### Not Executed

Important validation that was not performed.

Example:

```text
- Full integration test suite
  Not executed during this session.
```

### Manual Verification

If behavior was manually inspected or reproduced, describe what was checked.

Never imply that unexecuted validation passed.

---

## 7. Risks and Open Questions

Identify issues that a reviewer should be aware of before merging.

Include:

- known risks
- unverified assumptions
- edge cases not covered by tests
- potentially fragile behavior
- incomplete work
- intentionally deferred changes
- external dependencies
- migration concerns
- backwards compatibility concerns

Do not invent risks merely to fill the section.

If no meaningful risks were identified, state that explicitly.

---

## 8. PR Review Guide

Give practical guidance for reviewing the resulting changes.

Highlight the parts that deserve attention.

For each item, explain why it deserves attention.

Good examples:

```text
- Review the request validation in `src/...`.
  This is where malformed input is now rejected, and the behavior differs from the previous implementation.

- Review the transaction boundary in `src/...`.
  The implementation moved the database update into the same transaction as the related record creation.
```

Avoid generic advice such as:

```text
- Check that the code is correct.
- Review the tests.
- Make sure there are no bugs.
```

The review guide should tell the reviewer **where to look and why**.

---

## 9. Files and Areas Changed

Provide a concise map of the relevant changes.

Group files by purpose where possible.

Example:

```text
src/foo/service.ts
  Main implementation of the new behavior.

src/foo/service.test.ts
  Tests for the new behavior and edge cases.

src/config/foo.ts
  Configuration required by the new implementation.
```

Do not include every generated or incidental file unless it matters to the review.

---

## 10. Merge Checklist

Create a short checklist containing concrete things a human reviewer should verify before merging.

Only include checks relevant to this session.

Examples:

```text
- [ ] Confirm the new behavior matches the intended product requirement.
- [ ] Review the changed transaction boundary.
- [ ] Confirm the new edge-case tests cover the expected failure mode.
- [ ] Decide whether the unverified integration test should be run before merge.
```

Do not use the checklist as a generic software-engineering checklist.

---

# Writing Guidelines

## Use plain language

Prefer:

> The API now rejects an invalid token before creating the database record.

Over:

> Added pre-persistence validation to improve request processing integrity.

Prefer concrete verbs:

- added
- removed
- changed
- moved
- validates
- rejects
- creates
- reads
- updates
- deletes

Avoid unnecessary corporate or abstract language.

## Explain technical terms when needed

If a repository-specific term is important, briefly explain it the first time it appears.

For example:

> The request passes through the "command handler", which is the layer responsible for validating the request and coordinating the database operation.

Do not define common programming terms unnecessarily.

## Separate facts from interpretation

Clearly distinguish:

- what the code does
- what the session discovered
- why a decision was made
- what remains uncertain

Do not present assumptions as facts.

## Prefer evidence over narrative

The report should explain the session, but it should not become a transcript.

Do not include:

- every shell command
- every file inspected
- every failed search
- repetitive implementation details
- internal reasoning that does not help review the result

Include the evidence and events that explain the final implementation.

## Be honest about incomplete work

If the task was only partially completed, say so clearly.

Use language such as:

> The implementation is complete, but the integration test could not be executed in this environment.

or:

> The session changed the validation path, but the downstream behavior was not manually verified.

Do not hide incomplete validation in vague language.

---

# Output Rules

Unless the user explicitly asks for another format:

- Output Markdown.
- Use the report structure defined above.
- Keep the TL;DR short.
- Prefer concise explanations over exhaustive detail.
- Include code or command snippets only when they materially improve understanding.
- Use repository-relative file paths.
- Use exact test/validation results when available.
- Do not modify source code, tests, configuration, or unrelated files.
- Do not create a report file unless the user explicitly requests one.

If the user asks to save the report, choose an appropriate repository location based on existing project conventions. Do not invent a documentation directory when the repository already has an established convention.

---

# Accuracy Rules

Before finalizing the report, perform a consistency check.

Verify that:

- every claimed change corresponds to an actual change or clearly established session action
- every claimed test result corresponds to an actually executed test
- pre-existing changes are not incorrectly attributed to this session
- important implementation decisions are supported by session evidence
- unresolved issues are clearly identified
- the report describes the final state rather than an earlier intermediate state
- the report does not claim certainty where the evidence is incomplete

If the session context and repository state disagree, investigate the discrepancy before writing the final report.

If the discrepancy cannot be resolved, explicitly describe the uncertainty.

---

# Final Quality Test

Before presenting the report, ask:

> Could a developer who did not participate in this session read this report and understand what changed, why it changed, how the behavior changed, what was actually verified, and what they should inspect before merging?

If the answer is no, improve the report.

The report is successful when it gives a reviewer the context that a raw diff cannot provide.