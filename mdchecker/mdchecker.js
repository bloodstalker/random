#!/usr/bin/env node
"use strict"
const fs = require("fs")
const jsdom = require("jsdom")
const {JSDOM} = jsdom;

const mitTexMath = require("markdown-it-texmath")
const mitMulMd = require("markdown-it-multimd-table")
const mit = require("markdown-it")({html:true})
  .enable(["table"])
  .disable(["strikethrough"])
  .use(mitTexMath, {
    engine: require("katex"),
    delimiters: "gitlab",
    katexOptions: {macros: {"\\RR":"\\mathbb{R}"}}})
  .use(mitMulMd)

const ArgumentParser = require("argparse").ArgumentParser
const parser = new ArgumentParser({})

parser.addArgument(["--xml"], {
  help: "path to the xml file",
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
  const r = /<(?<parent>.*)>\<\!\[CDATA\[(?<cdata>(.|\n?|(\r\n)?)*?)\]\]\>/g
  const matches = xmlSerialized.matchAll(r)
  for (let match of matches){
    let {parent, cdata} = match.groups
    if (("Description" != parent) && ("Postcondition" != parent) && ("Precondition" != parent) && ("Remark" != parent)) {
      console.log("CDATA appears in illegal node type.")
      process.exit(1)
    } else {
      const res = mit.render(cdata)
      console.log(res)
    }
  }
}

mdchecker()
