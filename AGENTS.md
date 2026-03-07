# AGENTS.md

## Repository agent rules

- ENGLISH ONLY in all agent-user communication.
- NO TESTING: Do not run automated tests, build checks, or manual testing unless the user explicitly requests testing.
- DEFAULT DELIVERY POLICY (unless the user says otherwise):
  - After code changes, build the project artifact(s) that correspond to the change.
  - Include and commit source changes + related docs updates + generated artifact outputs together.
  - Push results directly to `main`.

## Cursor Cloud specific instructions

- Build execution strategy (unless user says otherwise):
  - Run all required builds in the cloud VM agent environment, not on the developer's machine.
  - Use additional build flags/settings explicitly provided by the developer in the conversation.
  - Upload resulting runnable artifacts into the repository together with source/docs changes.
  - Provide developer-ready launch scripts/instructions so binaries and hosted client files can be run directly on the developer VM.
