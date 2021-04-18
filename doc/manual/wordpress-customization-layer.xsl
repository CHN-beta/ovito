<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:import href="docbook-xsl/html/chunk.xsl"/>

  <xsl:output method="html" encoding="ISO-8859-1" indent="yes" />

  <xsl:param name="callout.graphics" select="'0'"/>
  <xsl:param name="callout.unicode" select="'1'"/>

  <xsl:param name="wordpress.dir">../..</xsl:param>
  <xsl:param name="use.extensions" select="0"/>

  <xsl:param name="chunk.section.depth" select="3"/>
  <xsl:param name="chunk.first.sections" select="1"/>
  <xsl:param name="chunker.output.encoding" select="'UTF-8'"/>
  <xsl:param name="use.id.as.filename" select="1"/>
  <xsl:param name="html.stylesheet" select="'manual-wordpress.css'"/>
  <xsl:param name="chapter.autolabel" select="0"/>
  <xsl:param name="section.autolabel" select="0"/>
  <xsl:param name="section.label.includes.component.label" select="1"/>
  <xsl:param name="xref.with.number.and.title" select="0"/>
  <xsl:param name="suppress.footer.navigation" select="0"/>
  <xsl:param name="header.rule" select="0"/>
  <xsl:param name="footer.rule" select="1"/>
  <xsl:param name="navig.showtitles" select="1"/>
  <xsl:param name="generate.id.attributes" select="1"/>
  <xsl:param name="highlight.source" select="1"/>
  <xsl:param name="css.decoration" select="0" />
  <xsl:param name="table.borders.with.css" select="0" />
  <xsl:param name="html.ext" select="'.php'" />

  <xsl:param name="generate.toc">
  appendix  toc,title
  article/appendix  nop
  article   toc,title
  book      toc,title
  chapter   nop
  part      nop
  preface   nop
  qandadiv  nop
  qandaset  nop
  reference toc,title
  sect1     nop
  sect2     nop
  sect3     nop
  sect4     nop
  sect5     nop
  section   nop
  set       toc
  </xsl:param>

  <xsl:template match="section[@role = 'NotInToc']"  mode="toc" />

  <xsl:template name="gentext.nav.home">Table of Contents</xsl:template>

  <xsl:template match="figure[@role = 'inline']" mode="class.value">
    <xsl:value-of select="'inlinefigure'"/>
  </xsl:template>
  <xsl:template match="informalfigure[@role = 'inline']" mode="class.value">
    <xsl:value-of select="'inlinefigure'"/>
  </xsl:template>

  <!-- Template for an element that generates a tag for marking program features of the PRO version -->
  <xsl:template match="ovito-pro" name="ovito-pro">
    <xsl:variable name="pro.description.page" select="//section[@id='credits.ovito_pro']"/>
    <a class="ovito-pro-tag" data-tooltip="This program feature is only available in the Pro edition of OVITO. Click to learn more." data-tooltip-position="bottom">
      <xsl:attribute name="href">https://www.ovito.org/about/ovito-pro/</xsl:attribute>
    <!--
      <xsl:attribute name="href">
        <xsl:call-template name="href.target">
          <xsl:with-param name="object" select="$pro.description.page" />
        </xsl:call-template>
      </xsl:attribute>
    -->
      pro
    </a>
  </xsl:template>

  <!-- Template for an element that generates a link into the Sphinx-generated Python API documentation -->
  <xsl:template match="pydoc-link">
    <xsl:param name="href" select="@href" />
    <xsl:param name="anchor" select="@anchor" />
    <xsl:param name="no-pro-tag" select="@no-pro-tag" />
    <xsl:call-template name="link">
      <xsl:with-param name="xhref" select="concat('python/', @href, '.php', '#', @anchor)"/>
    </xsl:call-template>
    <xsl:if test="not($no-pro-tag)">
      <xsl:call-template name="ovito-pro" />
    </xsl:if>
  </xsl:template>

  <xsl:template name="chunk-element-content">
    <xsl:param name="prev"/>
    <xsl:param name="next"/>
    <xsl:param name="nav.context"/>
    <xsl:param name="content">
      <xsl:apply-imports/>
    </xsl:param>
    <xsl:variable name="doc" select="self::*"/>
    <xsl:processing-instruction name="php">
  header("Location: https://www.ovito.org/docs/current/index.html", true, 303);
  die();
?</xsl:processing-instruction>
  </xsl:template>

</xsl:stylesheet>