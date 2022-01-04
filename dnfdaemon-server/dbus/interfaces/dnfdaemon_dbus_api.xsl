<?xml version = "1.0" encoding = "UTF-8"?>
<xsl:stylesheet version = "1.0" xmlns:xsl = "http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text" omit-xml-declaration="yes" indent="no"/>

<xsl:variable name="newline"><xsl:text>
</xsl:text></xsl:variable>

<xsl:template match="/">
..
    Copyright Contributors to the libdnf project.

    This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

    Libdnf is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Libdnf is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdnf.  If not, see &lt;https://www.gnu.org/licenses/&gt;.


########################
 DBus API documentation
########################


Synopsis
========

Dbus API doc

Generated automatically using xslt


Description
===========


Interfaces
==========
<xsl:for-each select="/interface_list/file">
<xsl:apply-templates select="document(@name)/node" />
</xsl:for-each>
</xsl:template>

<xsl:template match="node">
<xsl:apply-templates />
</xsl:template>

<xsl:template match="interface">
<xsl:value-of select="@name"/>
`````
<xsl:for-each select="method">
.. <xsl:value-of select="@name"/>
<xsl:for-each select="arg">
<xsl:value-of select="$newline" />
<xsl:value-of select="@name" />
<xsl:value-of select="$newline" />
<xsl:for-each select="annotation">
<xsl:value-of select="@value" />
</xsl:for-each>
type:  <xsl:value-of select="@type" />
direction:  <xsl:value-of select="@direction" />
<xsl:value-of select="$newline" />

</xsl:for-each>
<xsl:for-each select="annotation">
<xsl:value-of select="$newline" />
<xsl:value-of select="@value"/>
</xsl:for-each>
</xsl:for-each>
</xsl:template>
</xsl:stylesheet>
