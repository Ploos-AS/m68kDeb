# Release manifest schema (M0)

Release manifests are YAML files under `releases/`.

Required M0 keys:

```yaml
schema: 1
id: woody
name: Debian GNU/Linux 3.0
codename: woody
version: "3.0"
class: historical-official
security_status: obsolete
platforms: [amiga, atari, mac68k]
installer_strategy: historical-native
source:
  type: debian-archive
  notes: "Pin exact artifacts during qualification."
qualification: unverified
```

Allowed `class` values: `historical-official`, `historical-unofficial`, `current-ports`.

Allowed `qualification` values: `qualified`, `unverified`, `blocked`, `unsupported`.

The schema will become machine-validated before M1 qualification.
