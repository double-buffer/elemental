xattr -l /path/to/application.app
You'll probably find com.apple.quarantine listed. If so, you can get rid of it with

xattr -dr com.apple.quarantine /path/to/application.app

https://docs.avaloniaui.net/docs/deployment/macOS
https://github.com/Kong/insomnia/blob/c4dff179daa1895246edbbe335c58d559167f761/.github/workflows/release-build.yml#L169
