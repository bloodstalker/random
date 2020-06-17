#!/usr/bin/node
//npm install argparse
//npm install markdown-it
//npm install jsdom
"use strict"
const fs = require("fs")
const jsdom = require("jsdom")
const {JSDOM} = jsdom;

const mit = require("markdown-it")
const mt = new mit()

const ArgumentParser = require("argparse").ArgumentParser
const parser = new ArgumentParser({})

parser.addArgument(["--xml"], {
  help: "path to xml file",
  required: true,
  dest: "xmlPath"
})

function mdchecker (){
  const args = parser.parseArgs()
  const txt = fs.readFileSync(args.xmlPath, "utf-8")
  const dom = new JSDOM(txt, {
    contentType:"text/xml"
  })
  const xmlSerialized = dom.serialize()
  const r = /\<\!\[CDATA\[(?<cdata>.*)\]\]\>/g
  const matches = xmlSerialized.matchAll(r)
  for (let match of matches){
    let cdata = match.groups
    console.log(cdata)
  }

}

mdchecker()
