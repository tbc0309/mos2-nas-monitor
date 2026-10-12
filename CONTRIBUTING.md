# Contributing

Start with the [firmware development guide](docs/DEVELOPMENT.md), [module map](docs/STRUCTURE.md), and [SNMP adapters](docs/TELEMETRY.md). Simulator development belongs in [simulator/README.md](simulator/README.md).

- Keep changes focused and describe the user-visible behavior and validation.
- Use English identifiers; keep user-facing strings in the locale sources. Explain lifecycle constraints rather than commenting every statement.
- Verify protocol, state and resource changes locally. Run the resource generators and `pio run` before submitting; the repository CI verifies generated resources and firmware/filesystem builds.
- A new SNMP adapter needs real OID/type evidence, bounded requests and unsupported/error cases. Dashboard data is not evidence that the SNMP agent exports the same metric.
- Preserve configuration/photo ownership and keep network work out of drawing code.
- Never commit credentials, private configurations, device logs, local executables or firmware build output.

Report bugs with firmware version, device/OS, SNMP version, steps to reproduce and redacted diagnostics. Use [Issues](https://github.com/tbc0309/mos2-nas-monitor/issues) for ordinary bugs. For vulnerabilities, use the repository's private vulnerability reporting option if enabled; do not post passwords or exploit details publicly.

Original project code uses the [MIT License](LICENSE); third-party assets retain their own licenses in [NOTICE](NOTICE.md).
