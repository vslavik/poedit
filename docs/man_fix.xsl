<?xml version="1.0"?>

<!-- This stylesheet fixes a problem with our use of AsciiDoc: we
     want to use <cmdsynopsis> for command synopsis. -->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <xsl:output method="xml"
                doctype-system="http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd"
                doctype-public="-//OASIS//DTD DocBook XML V4.2//EN"/>

    <xsl:template match="node()|@*">
        <xsl:copy>
            <xsl:apply-templates select="node()|@*"/>
        </xsl:copy>
    </xsl:template>

    <xsl:template match="refsynopsisdiv/simpara">
        <cmdsynopsis>
            <xsl:apply-templates/>
        </cmdsynopsis>
    </xsl:template>

</xsl:stylesheet>
