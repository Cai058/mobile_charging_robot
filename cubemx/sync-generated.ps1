[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidateSet('can')]
    [string]$Peripheral = 'can'
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$mappings = @{
    can = @(
        @{
            Source = Join-Path $PSScriptRoot 'generated/mobile_charging_robot_cubemx/Src/can.c'
            Destination = Join-Path $repoRoot 'User/Src/can.c'
        }
    )
}

foreach ($mapping in $mappings[$Peripheral]) {
    $source = $mapping.Source
    $destination = $mapping.Destination

    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "CubeMX generated file not found: $source. Run Generate Code first."
    }

    $sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    $destinationHash = if (Test-Path -LiteralPath $destination -PathType Leaf) {
        (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
    } else {
        $null
    }

    if ($sourceHash -eq $destinationHash) {
        Write-Host "Already synchronized: $destination"
        continue
    }

    if ($PSCmdlet.ShouldProcess($destination, "Copy CubeMX generated file from $source")) {
        Copy-Item -LiteralPath $source -Destination $destination -Force

        $copiedHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
        if ($copiedHash -ne $sourceHash) {
            throw "SHA-256 verification failed after copy: $destination"
        }

        Write-Host "Synchronized: $source -> $destination"
        Write-Host "SHA-256: $sourceHash"
    }
}
