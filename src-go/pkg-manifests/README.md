# pkg-manifests Utility
This is a simple go utility written to scan various package manifest logs and generate a markdown file of the recent changes

## To Build
1. Install the "go" package
2. Run `go build pkg-manifests.go` to create the "stats-scan" binary

## To Run
Usage: `pkg-manifests <old_manifest> <new_manifest>`

This will print all the text to the terminal, so if you want to save the results to a file instead you will want to pipe it into a file:
`pkg-manifests <old_manifest> <new_manifest> > results.md`
