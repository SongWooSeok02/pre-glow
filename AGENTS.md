# pre-glow development

Follow the phases in docs/architecture.md in order. Ask the user before a major architecture change. Keep audio callbacks free of allocation, blocking locks, and file I/O. Test DSP independently and record only build and validation results actually obtained. Preserve plugin and parameter IDs and document state migrations. Review dependency licensing before distribution.

Track development in GitHub. Commit each verified development step with a descriptive message, update CHANGELOG.md for user-visible changes, and record platform verification in README.md. Push completed steps to the configured origin and check the corresponding GitHub Actions results.
