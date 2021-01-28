<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:ng="http://docbook.org/docbook-ng"
	xmlns:db="http://docbook.org/ns/docbook"
	xmlns:exsl="http://exslt.org/common"
  xmlns:xlink='http://www.w3.org/1999/xlink'
	version="1.0"
	exclude-result-prefixes="exsl db ng xlink">

<!-- ================================================================================== -->
<!-- Customizations of the chunked HTML style for the generation of the HTML formatted manual. -->
<!-- ================================================================================== -->

<xsl:import href="docbook-xsl/html/chunk.xsl"/>

<xsl:param name="chunk.section.depth" select="3"/>
<xsl:param name="chunk.first.sections" select="1"/>
<xsl:param name="use.id.as.filename" select="1"/>
<xsl:param name="html.stylesheet" select="'manual.css'"/>
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
<xsl:param name="table.borders.with.css" select="1" />

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
    <xsl:with-param name="xhref" select="concat('python/', @href, '.html', '#', @anchor)"/>
  </xsl:call-template>
  <xsl:if test="not($no-pro-tag)">
    <xsl:call-template name="ovito-pro" />
  </xsl:if>
</xsl:template>

<xsl:template name="header.navigation">
  <xsl:param name="prev" select="/foo"/>
  <xsl:param name="next" select="/foo"/>
  <xsl:param name="nav.context"/>

  <xsl:variable name="home" select="/*[1]"/>
  <xsl:variable name="up" select="parent::*"/>

  <xsl:variable name="row1" select="$navig.showtitles != 0"/>
  <xsl:variable name="row2" select="count($prev) &gt; 0
                                    or (count($up) &gt; 0
                                        and generate-id($up) != generate-id($home)
                                        and $navig.showtitles != 0)
                                    or count($next) &gt; 0"/>

  <xsl:if test="$suppress.navigation = '0' and $suppress.header.navigation = '0'">
    <div class="navheader">
    	<a href="https://www.ovito.org/"><img src="images/ovito_logo.png" border="0" width="84" /></a>
      <xsl:if test="$row1 or $row2">
        <table width="100%" summary="Navigation header">
          <xsl:if test="$row1">
            <tr>
              <td width="20%" align="{$direction.align.start}">
                <xsl:choose>
                  <xsl:when test="$home != . or $nav.context = 'toc'">
                    <a accesskey="h">
                      <xsl:attribute name="href">
                        <xsl:call-template name="href.target">
                          <xsl:with-param name="object" select="$home"/>
                        </xsl:call-template>
                      </xsl:attribute>
                      <xsl:call-template name="navig.content">
                        <xsl:with-param name="direction" select="'home'"/>
                      </xsl:call-template>
                    </a>
                    <xsl:if test="$chunk.tocs.and.lots != 0 and $nav.context != 'toc'">
                      <xsl:text>&#160;|&#160;</xsl:text>
                    </xsl:if>
                  </xsl:when>
                  <xsl:otherwise>&#160;</xsl:otherwise>
                </xsl:choose>
              </td>
              <th width="60%" align="center">
                <xsl:apply-templates select="." mode="object.title.markup"/>
              </th>
              <td width="20%" align="{$direction.align.end}">
				<xsl:choose>
                  <xsl:when test="count($up)&gt;0
                                  and generate-id($up) != generate-id($home)">
                    <a accesskey="u">
                      <xsl:attribute name="href">
                        <xsl:call-template name="href.target">
                          <xsl:with-param name="object" select="$up"/>
                        </xsl:call-template>
                      </xsl:attribute>
                      <xsl:call-template name="navig.content">
                        <xsl:with-param name="direction" select="'up'"/>
                      </xsl:call-template>
                    </a>
                  </xsl:when>
                  <xsl:otherwise>&#160;</xsl:otherwise>
                </xsl:choose>
              </td>
            </tr>
          </xsl:if>

          <xsl:if test="$row2">
            <tr>
              <td width="20%" align="{$direction.align.start}">
                <xsl:if test="count($prev)>0">
                  <a accesskey="p">
                    <xsl:attribute name="href">
                      <xsl:call-template name="href.target">
                        <xsl:with-param name="object" select="$prev"/>
                      </xsl:call-template>
                    </xsl:attribute>
                    <xsl:call-template name="navig.content">
                      <xsl:with-param name="direction" select="'prev'"/>
                    </xsl:call-template>
                  </a>
                </xsl:if>
                <xsl:text>&#160;</xsl:text>
              </td>
              <th width="60%" align="center">
                <xsl:choose>
                  <xsl:when test="count($up) > 0
                                  and generate-id($up) != generate-id($home)
                                  and $navig.showtitles != 0">
                    <xsl:apply-templates select="$up" mode="object.title.markup"/>
                  </xsl:when>
                  <xsl:otherwise>&#160;</xsl:otherwise>
                </xsl:choose>
              </th>
              <td width="20%" align="{$direction.align.end}">
                <xsl:text>&#160;</xsl:text>
                <xsl:if test="count($next)>0">
                  <a accesskey="n">
                    <xsl:attribute name="href">
                      <xsl:call-template name="href.target">
                        <xsl:with-param name="object" select="$next"/>
                      </xsl:call-template>
                    </xsl:attribute>
                    <xsl:call-template name="navig.content">
                      <xsl:with-param name="direction" select="'next'"/>
                    </xsl:call-template>
                  </a>
                </xsl:if>
              </td>
            </tr>
          </xsl:if>
        </table>
      </xsl:if>
      <xsl:if test="$header.rule != 0">
        <hr/>
      </xsl:if>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="user.footer.content">
  <p class="footer_copyright">&#x00A9; 2021  OVITO GmbH</p>
</xsl:template>

</xsl:stylesheet>