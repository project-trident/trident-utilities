package main

import (
	"os/exec"
	"bufio"
	"fmt"
	"os"
	"strings"
	"io/ioutil"
)

func fileExists(filename string) bool {
    info, err := os.Stat(filename)
    if os.IsNotExist(err) {
        return false
    }
    return !info.IsDir()
}

func setupSystem(){
  needrestart := false
  err := os.MkdirAll( "/etc/autofs/auto.master.d/", 0755)
  if err != nil { fmt.Println("Could not setup autofs rules! Check if this is being run as root?", err) ; os.Exit(1) }
  if !fileExists("/etc/autofs/auto.trident") {
    // Need to create the autofs rule to let users mount devices
    err = ioutil.WriteFile("/etc/autofs/auto.trident",[]byte("*	-fstype=auto,rw,nosuid,user=${USER},gid=users	:/dev/& \n"), 0644)
    needrestart = true
  }
  if err != nil { fmt.Println("Could not setup autofs rules! Check if this is being run as root?", err) ; os.Exit(1) }
  if !fileExists("/etc/autofs/auto.master.d/trident.autofs") {
    //Need to create the autofs dir rule to map to the devices
    err = ioutil.WriteFile("/etc/autofs/auto.master.d/trident.autofs", []byte("/browse 	/etc/autofs/auto.trident	--timeout=5 \n"), 0644)
    needrestart = true
  }
  if err != nil { fmt.Println("Could not setup autofs rules! Check if this is being run as root?", err) ; os.Exit(1) }
  //Now restart the autofs service if needed
  if needrestart {
    cmd := exec.Command("sv", "restart", "autofs")
    cmd.Run()
  }
}



func handleEvent(line string){
  if !strings.HasPrefix(line, "UDEV") { return } //not a valid event
  entries := strings.Split(line, " ")
  if len(entries) < 5 { return } //not a valid entry - possibly a startup message
  var deviceID string
  var eventType string
  for _, word := range(entries){
    if word == "" { continue }
    if word == "add" || word == "remove" {
      eventType = word
    }else if strings.HasPrefix(word, "/devices/") {
      path := strings.Split(word,"/")
      deviceID = path[len(path)-1] //last entry on the path
    }
  }
  if deviceID == "" || eventType == "" { return } //not a valid event
  //fmt.Println("Got Device Event:", eventType, deviceID)
  entry := "/media/"+deviceID+".desktop"
  if eventType == "add" {
    go createEntry(deviceID, entry)
  } else {
    if fileExists(entry) {
      os.Remove(entry)
    }
  }
}

func createEntry(device string, filepath string) {
  //First get the info about this device
  bytes, err := exec.Command("udevadm","info","/dev/"+device).Output()
  if err != nil { return } //no info about this device - was it removed between the event and now?
  var FS string
  var Model string
  var Vendor string
  lines := strings.Split( string(bytes), "\n" )
  for _, line := range(lines) {
    list := strings.Split(line,"=")
    if len(list) != 2 { continue }
    //fmt.Println("Got Line", list)
    id := strings.Split(list[0], " ")[1]
    switch id {
      case "ID_FS_TYPE": FS = list[1]
      case "ID_MODEL": Model = list[1]
      case "ID_VENDOR": Vendor = list[1]
    }
  }
  if FS == "" { return } //if the FS cannot be detected, then it is a good bet it cannot be browsed/used
  // Now assemble the output file content
  var content []string
  content = append(content, "[Desktop Entry]")
  content = append(content, "Version=1.1")
  content = append(content, "Type=Directory")
  content = append(content, "Path=/browse/"+device+"\n")
  content = append(content, "Name="+Vendor+" "+Model)
  content = append(content, "Comment="+device+" ("+FS+")")
  switch FS {
    case "cd9660": content = append(content, "Icon=media-optical")
    default: content = append(content, "Icon=media-removable")
  }

  // And save the output file
  ioutil.WriteFile( filepath, []byte( strings.Join(content, "\n") ), 0644 )
}

func main() {
  setupSystem()
  cmd := exec.Command("udevadm","monitor","-u","-s", "block")
  stdout, err := cmd.StdoutPipe()
  if(err != nil){ fmt.Println("Error initializing udevadm monitor pipe:", err) ; os.Exit(1) }
  scanner := bufio.NewScanner(stdout)
  scanner.Split(bufio.ScanLines)
  go func() {
    for scanner.Scan() {
      //Got an event from udev
      handleEvent(scanner.Text())
    }
  }()
  err = cmd.Start()
  if(err != nil){ fmt.Println("Error starting udevadm monitor:", err) ; os.Exit(1) }
  cmd.Wait() //wait for udevadm process to exit (on system reboot or service shutdown)
}
