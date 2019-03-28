package main

import (
	"fmt"
	"bufio"
	"os"
	"os/exec"
	S "strings"
	"regexp"
	"strconv"
	"net/url"
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
    /* Ignore invalid lines or scans by bots */
    if elem[0] == "" || S.Contains( S.ToLower(elem[3]), "bot") { continue }
    /* Now add this entry into the hash */
    add_to_hash(elem, hash)
  }
  file.Close()
  if tmpfile != "" {
    os.Remove(tmpfile)
  }
}

func parse_line(text string) []string {
  /* Function to parse a pkg.list line into: */
  /* origin, pkgname, version */
  line := make([]string, 3)
  line = S.Split(text, " : ")
  return line
}

func add_to_hash( elem []string, hash map[string]string) {
  /* elem = [origin, pkgname, version] */
  combo := elem[0]+","+elem[2]
  hash[elem[1]] = combo
}

func print_hash(hash map[string]string){
  fmt.Println("{")
  first := true
  val := ""
  for key := range hash {
    val = ""
    if first != true { 
      val = ","
    } else { 
      first = false
    }
    val = val+"\""+key+"\" : "+ strconv.Itoa( num_unique_items( S.Split(hash[key],",") ) )
    fmt.Println(val)
  }
  fmt.Println("}")
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
    vals := S.Split( hash[key], ",")
    line := "* "+key + " ("+vals[0]+") : "+vals[1]
    fmt.Println(line) 
  }
}

func compare_files(prev map[string]string, now map[string]string) {
  newpkg := make(map[string]string);
  delpkg := make(map[string]string);
  uppkg := make(map[string]string);
  /* Now iterate through the old packages */
  for key, val := range prev {
    if nowval, ok := now[key]; ok {
      /* Package still exists */
      if val != nowval {
        verchange := S.Split(val,",")[1] + " -> " +S.Split(newval,",")[1]
        uppkg[key] = S.Join( S.Split(val,",")[0], verchange)
      }
    } else {
      /* Package no longer available */
      delpkg[key] = val
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
  fmt.Println("## New Packages ("+ len(newpkg)+")")
  print_map(newpkg)
  fmt.Println("")
  fmt.Println("## Deleted Packages ("+ len(delpkg)+")")
  print_map(delpkg)
  fmt.Println("")
  fmt.Println("## Updated Packages ("+ len(uppkg)+")")
  print_map(uppkg)

}

func main() {
  oldlist := make(map[string]string)
  nowlist := make(map[string]string)
  if os.Args.size() < 3 {
    fmt.Println("Invalid Inputs: Need <oldlist> <newlist>")
    panic(1);
  }
  read_file(os.Args[1], oldlist)
  read_file(os.Args[2], nowlist)
  compare_files(oldlist, nowlist);
}
