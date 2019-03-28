package main

import (
	"fmt"
	"bufio"
	"os"
	"os/exec"
	S "strings"
/*	"regexp"*/
	"strconv"
/*	"net/url"*/
	"sort"
)

func exit_error(e error) {
  if e != nil {
    panic(e)
  }
}

func read_file(path string, hash map[string]string) {
  tmpfile := ""
  if S.HasSuffix(path, ".bz2") {
    tmpfile = S.TrimSuffix(path, ".bz2")
    cmd := exec.Command("bunzip2", "-k", path)
    if cmd.Run() != nil { 
      if _, err := os.Stat(tmpfile); os.IsNotExist(err) {
        fmt.Println("Could not unzip file:", path)
        return
      }
    }
    path = tmpfile
  }

  file, err := os.Open(path)
  exit_error(err)
  reader := bufio.NewReader(file)
  scanner := bufio.NewScanner(reader)
  for scanner.Scan() {
    elem := parse_line(scanner.Text())
    /* Ignore invalid lines */
    if len(elem) != 3 { continue }
    /* Now add this entry into the hash */
    add_to_hash(elem, hash)
  }
  file.Close()
  if tmpfile != "" {
    os.Remove(tmpfile)
  }
}

func read_moved(path string, hash map[string]string) {
  file, err := os.Open(path)
  exit_error(err)
  reader := bufio.NewReader(file)
  scanner := bufio.NewScanner(reader)
  for scanner.Scan() {
    elem := S.Split(scanner.Text(),"|")
    /* Ignore invalid lines */
    if len(elem) < 3 { continue }
    if S.HasPrefix(elem[0], "#") { continue }
    /* Now add this entry into the hash */
    hash[elem[0]] = elem[1]+",["+elem[2]+"] "+elem[3]
  }
  file.Close()
}

func parse_line(text string) []string {
  /* Function to parse a pkg.list line into: */
  /* origin, pkgname, version */
  line := S.Split(text, " : ")
  if len(line) == 2 {
    var s []string
    line = append(s , "", line[0], line[1])
  }
  return line
}

func add_to_hash( elem []string, hash map[string]string) {
  /* elem = [origin, pkgname, version] */
  combo := elem[0]+","+elem[2]
  hash[elem[1]] = combo
}

func sort_keys(hash map[string]string) []string {
  keys := make([]string, 0, len(hash))
  for key := range hash {
    keys = append(keys, key)
  }
  sort.Strings(keys)
  return keys;
}

func print_map(hash map[string]string) {
  keys := sort_keys(hash);
  for key := range keys {
    vals := S.Split( hash[keys[key]], ",")
    line := ""
    if vals[0] != "" {
      line = "* "+keys[key] + " ("+vals[0]+") : "+vals[1]
    } else if vals[1] != "" {
      line = "* "+keys[key] + " : "+vals[1]
    } else {
      line = "* "+keys[key]
    }
    fmt.Println(line) 
  }
}

func compare_files(prev map[string]string, now map[string]string, moved map[string]string) {
  newpkg := make(map[string]string);
  delpkg := make(map[string]string);
  uppkg := make(map[string]string);
  /* Now iterate through the old packages */
  for key, val := range prev {
    if nowval, ok := now[key]; ok {
      /* Package still exists */
      if S.Split(val,",")[1] != S.Split(nowval,",")[1] {
        /* New Version */
        verchange := S.Split(val,",")[1] + " -> " +S.Split(nowval,",")[1]
        uppkg[key] = S.Split(val,",")[0]+","+verchange
      }
    } else {
      /* Package no longer available */
      origin := S.Split(val,",")[0]
      info := ""
      if origin != "" {
        /* Load any information about why this port is not available */
        if movedval, ok := moved[origin]; ok {
          mval := S.Split(movedval, ",")
          if mval[0] == "" {
            info = mval[1]
          } else {
            info = "Moved to " +mval[0]+". "+mval[1]
          }
        }
      }
      delpkg[key] = origin+","+info
    }
  }
  /* Now iterate through the new packages */
  for key, val := range now {
    if _, ok := prev[key]; !ok {
      /* New package not in old list */
      newpkg[key] = val
    }
  }

  /* Now print out the results */
  if len(newpkg) > 0 {
    fmt.Println("## New Packages ("+ strconv.Itoa(len(newpkg))+")")
    print_map(newpkg)
    fmt.Println("")
  }
  if len(delpkg) > 0 {
    fmt.Println("## Deleted Packages ("+ strconv.Itoa(len(delpkg))+")")
    print_map(delpkg)
    fmt.Println("")
  }
  if len(uppkg) > 0 {
    fmt.Println("## Updated Packages ("+ strconv.Itoa(len(uppkg))+")")
    print_map(uppkg)
  }
}

func main() {
  oldlist := make(map[string]string)
  nowlist := make(map[string]string)
  movedlist := make(map[string]string)
  if len(os.Args) < 3 {
    fmt.Println("Invalid Inputs: Need <oldlist> <newlist>")
    os.Exit(1);
  }
  read_file(os.Args[1], oldlist)
  read_file(os.Args[2], nowlist)
  if len(os.Args) >3 {
    read_moved(os.Args[3], movedlist);
  }
  compare_files(oldlist, nowlist, movedlist);
}
