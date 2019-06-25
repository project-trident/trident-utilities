package main

import (
	"fmt"
	"bufio"
	"os"
	"os/exec"
	S "strings"
	"strconv"
	"regexp"
	"net/url"
	"encoding/json"
	"sort"
	"time"
	"io/ioutil"
)

type IdList struct {
  List map[string]bool
}

func addId( id IdList, newid string) IdList {
  nmap := id.List
  if _, ok := nmap[newid]; !ok {
    if nmap == nil { nmap = make(map[string]bool) }
    nmap[newid] = true
    id.List = nmap
    /*fmt.Println("Added:", newid)*/
  }
  /*id.List = nmap*/
  return id
}

/* Define all the data objects for assembling JSON for charts */
type ChartAxes struct {
  Title  string `json:"title"`
}
type AxesScale struct {
  StackMode string `json:"stackMode"`
}
type ChartPoints struct {
  X	string `json:"x"`
  Value int `json:"value"`
}
type ChartSeries struct {
  Name	string `json:"name"`
  SeriesType	string `json:"seriesType"`
  Data	[]ChartPoints `json:"data"`
}
type Chart struct {
  Title	  string `json:"title"`
  Type	string `json:"type"`
  Legend	bool `json:"legend"`
  BarsPadding	float64 `json:"barsPadding"`
  BarGroupsPadding	float64 `json:"barGroupsPadding"`
  XAxes		[]ChartAxes `json:"xAxes"`
  YAxes		[]ChartAxes `json:"yAxes"`
  YScale		AxesScale `json:"yScale"`
  Series	[]ChartSeries `json:"series"`
}
type ChartObj struct {
  Chart Chart `json:"chart"`
}

func exit_error(e error) {
  if e != nil {
    panic(e)
  }
}

func read_file(path string, hash map[string]IdList) {
  tmpfile := ""
  if S.HasSuffix(path, ".bz2") {
    tmpfile = S.TrimSuffix(path, ".bz2")
    cmd := exec.Command("bunzip2", "-k", path)
    if cmd.Run() != nil { 
      if _, err := os.Stat(tmpfile); os.IsNotExist(err) {
        /*fmt.Println("Could not unzip file:", path)*/
        return
      }
    }
    path = tmpfile
  } else if S.HasSuffix(path, ".xz") {
    tmpfile = S.TrimSuffix(path, ".xz")
    cmd := exec.Command("unxz", "-k", path)
    /*fmt.Println("Start Unzipping file:", path);*/
    if cmd.Run() != nil { 
      if _, err := os.Stat(tmpfile); os.IsNotExist(err) {
        /*fmt.Println("Could not unzip file:", path)*/
        return
      }
      /*fmt.Println("Done Unzipping file:", path);*/
    }
    path = tmpfile
  }

  file, err := os.Open(path)
  if err != nil { /*fmt.Println("Could not read file:", file);*/ return }
  scanner := bufio.NewScanner(file)
  buf := make([]byte, 0, 1024*1024)
  scanner.Buffer(buf, 10*1024*1024)
  line_num := 0
  start := time.Now()
  for scanner.Scan() {
    line_num = line_num+1
    elem := parse_line(scanner.Text())
    /*fmt.Println("Got elem:", elem)*/
    if (line_num%100000) == 0 { fmt.Println("Lines Read (x1000):", line_num/1000, "Elapsed:", time.Since(start)) }
    if elem[0] == "" || len(elem) != 4 { continue }
    add_to_hash(elem, hash)
  }
  file.Close()
  fmt.Println("Finished reading file. Total Lines:", line_num);
  if tmpfile != "" {
    /*os.Remove(tmpfile)*/
  }
}

func write_results_to_file(hash map[string]int, path string) {
  var content []string
  content = append(content, "{")
  for k := range hash {
    _jstext := make([]string, 4) 
    _jstext[0] = `"`
    _jstext[1] = k
    _jstext[2] = `":`
    _jstext[3] = strconv.Itoa(hash[k])
    if len(content) > 1 {  _jstext = append( []string{","}, _jstext...) }
    content = append(content, S.Join(_jstext, "") )
  }
  content = append(content, " }")
  ioutil.WriteFile(path, []byte( S.Join(content,"\n")), 0644)
}

func load_results_from_file( hash map[string]int, path string) {
  fmt.Println("Map size:", len(hash));
  jsfile, err := os.Open(path)
  if err == nil {
    _data, _ := ioutil.ReadAll(jsfile);
    json.Unmarshal([]byte(_data), &hash)
    jsfile.Close()
  } else { 
    fmt.Println("Could not open File:" , path);
  }
  fmt.Println("  After Import Map size:", len(hash));
}

func parse_line(text string) []string {
  /* Function to parse an nginx access log line into: */
  /* IP, Date, Path, Method */
  line := make([]string, 4)
  if S.Contains(text, " - - [") {
   words := regexp.MustCompile(`[^\s"']+|"([^"]*)"|'([^']*)`).FindAllString(text, -1)
   /* Pull out the IP */
    line[0] = S.Split(text, " ")[0]
  /* Pull out the date */
    line[1] = S.Split(text, "[")[1]
    line[1] = S.Split(line[1], ":")[0]
    /* Now convert from (dd,MMM,yyyy) to yyyy-MM-dd format */
    date, _ := time.Parse("02/Jan/2006", line[1])
    line[1] = date.Format("2006-01-02")
  /* Pull out the path */
    line[2] = S.Split(words[5], " ")[1]
      tmp, err := url.Parse(line[2])
      if err == nil { line[2] = tmp.EscapedPath() }
    line[2] = S.ToLower(line[2])
  /* Pull out the method */
    line[3] = S.Replace(words[9],"\"", "", -1)
  }
  return line
}

func add_to_hash( elem []string, hash map[string]IdList) {
  /* Daily index */
  combo := elem[1]+","+elem[3]
  hash[combo] = addId(hash[combo], elem[0])
  /* Total Daily index (all versions) */
  combo = elem[1]
  hash[combo] = addId(hash[combo], elem[0])
  /* Now Monthly index */
  month := S.Join( S.Split(elem[1],"-")[0:2], "-")
  combo = month+","+elem[3]
  hash[combo] = addId(hash[combo], elem[0])
  /* Now Total Monthly index (all versions)*/
  combo = month
  hash[combo] = addId(hash[combo], elem[0])
}

func hash_calc_unique( hash map[string]IdList) map[string]int {
  numhash := make(map[string]int)
  for k := range hash {
    numhash[k] = len( hash[k].List )
  }
  return numhash
}

func sorted_keys_from_hash( hash map[string]int) []string {
  sortedKeys := make([]string, 0, len(hash))
  for k := range hash {
    sortedKeys = append(sortedKeys, k)
  }
  sort.Strings(sortedKeys)
  return sortedKeys
}

func versions_from_hash( hash map[string]int, showdaily bool) []string {
  unique := make(map[string]bool)
  list := []string{}
  for key := range hash {
    elem := S.Split(key,","); /* [Date, Version] */
    if len(elem) <2 { continue }
    daily := (len( S.Split(elem[0],"-")) > 2)
    if  (daily && !showdaily) || (!daily && showdaily) { continue }
    if unique[elem[1]] != true {
      unique[elem[1]] = true
      list = append(list, elem[1])
    }
  }
  sort.Strings(list)
  return list
}

func hash_to_chart_object( hash map[string]int, showdaily bool) ChartObj {
  /* First assemble all the data series */
  _series := []ChartSeries{}
    /* First get the filter of all the versions */
    versions := versions_from_hash(hash, showdaily)
    keys := sorted_keys_from_hash(hash)
    /* Now find all the totals (non-versioned) */
    /* (do this first to ensure it is behind all other series on the graph)*/
    _total := ChartSeries{}
    _total.Name = "Total Unique"
    _total.SeriesType = "line"
    for _, key := range keys {
      elem := S.Split(key,","); /* [Date, Version] */
      if len(elem) < 2 { /* no version */
        daily := (len( S.Split(elem[0],"-")) > 2)
      if  (daily && !showdaily) || (!daily && showdaily) { continue }
        _total.Data = append( _total.Data, ChartPoints{ X: elem[0], Value: hash[key] } )
      }
    }
    _series = append(_series, _total)
 
    /* Now loop through all the versions and assemble the individual series */
  for _, version := range versions {
    _ser := ChartSeries{}
    _ser.Name = version
    _ser.SeriesType = "column"
    for _, key := range keys {
      elem := S.Split(key,","); /* [Date, Version] */
      if len(elem)>=2 && elem[1] == version {
        daily := (len( S.Split(elem[0],"-")) > 2)
        if  (daily && !showdaily) || (!daily && showdaily) { continue }
        _ser.Data = append( _ser.Data, ChartPoints{ X: elem[0], Value: hash[key] } )
      }
    }
    _series = append(_series, _ser)
  }
  /* Now assemble the Axes*/
  
  _xaxis := ChartAxes {}
  if showdaily { 
    _xaxis.Title = "Versions per Day" 
  } else { 
    _xaxis.Title = "Versions per Month"
  }
  _yaxis := ChartAxes { Title: "Unique Systems"}
  /* Finally assemble the chart and chartObject */
  _chart := Chart{}
    if showdaily {
      _chart.Title = "Unique Systems Per Day"
    } else {
      _chart.Title = "Unique System Per Month"
    }
    _chart.Type = "column"
    _chart.Legend = true
    _chart.BarsPadding = -0.5
    _chart.BarGroupsPadding = 2.0
    _chart.XAxes = []ChartAxes{ _xaxis }
    _chart.YAxes = []ChartAxes{ _yaxis }
    _chart.YScale = AxesScale{ StackMode: "value" }
    _chart.Series = _series

  cobj := ChartObj{}
    cobj.Chart = _chart

  /*js, _ := json.MarshalIndent(cobj, "", "  ")
  fmt.Println(string(js))*/
  return cobj
}

func print_json_obj( cobj ChartObj, path string){

  js, err := json.MarshalIndent(cobj,"","  ")
  if err == nil {
    fmt.Println("Writing JSON File:", path)
    ioutil.WriteFile(path, js, 0644)
  } else {
    fmt.Println("[ERROR] Could not create JSON")
  }

}

func main() {
  outfile := os.Args[2]

  numhash := make(map[string]int)
  if os.Args[1] == "analyze" {
    args := os.Args[3:]
    hash := make(map[string]IdList)
    for _, path := range args {
      /*fmt.Println(path)*/
      read_file(path, hash)
    }
    numhash = hash_calc_unique(hash)
    write_results_to_file(numhash, outfile);

  } else if os.Args[1] == "graph" {
    daily := os.Args[3]
    showdaily := (daily=="true" || daily=="daily")
    args := os.Args[4:]
    for _, path := range args {
      /*fmt.Println(path)*/
      load_results_from_file(numhash, path)
    }
    cobj := hash_to_chart_object(numhash, showdaily )
    print_json_obj( cobj , outfile)

  } else if os.Args[1] == "all" {
    /* This options does not use the intermediate JSON files */
    daily := os.Args[3]
    showdaily := (daily=="true" || daily=="daily")
    args := os.Args[4:]
    hash := make(map[string]IdList)
    for _, path := range args {
      /*fmt.Println(path)*/
      read_file(path, hash)
    }
    numhash = hash_calc_unique(hash)
    cobj := hash_to_chart_object(numhash, showdaily )
    print_json_obj( cobj , outfile)
  }
  /*print_hash(hash)*/
}
