# AGENTS.md

## Repository agent rules

- ENGLISH ONLY in all agent-user communication.
- NO TESTING: Do not run automated tests, build checks, or manual testing unless the user explicitly requests testing.
- DEFAULT DELIVERY POLICY (unless the user says otherwise):
  - After code changes, build the project artifact(s) that correspond to the change.
  - Include and commit source changes + related docs updates + generated artifact outputs together.
  - Push results directly to `main`.
